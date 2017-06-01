/*
 * MIT License
 *
 * Copyright (c) 2017 EDDR Software, LLC.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * Changes:
 * 2017-01-01: First & Last Name: What you did.
 * 2017-05-31: Kevin Nesmith: Initial contribution.
 */

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <memory.h>
#include <wordexp.h>
#include <linux/limits.h>
#include <boost/program_options.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace std;
using namespace boost::program_options;
using namespace boost::filesystem;
namespace fs = boost::filesystem;

bool iCompareChar(char a, char b)
{
    return tolower(a)==tolower(b);
}

bool iCompareString(const string &a, const string &b)
{
    if(a.length()==b.length()) {
        return equal(b.begin(), b.end(), a.begin(), iCompareChar);
    }

    return false;
}

int fullPath(const char *pathIn, const path &base, path *outPath)
{
    int retVal = 0;

    try {
        *outPath = canonical(pathIn, base);
    } catch(const filesystem_error &error) {
        if(is_symlink(error.path1())) {
            cerr << "ERROR: Path " << error.path1() << " contains a broken symlink." <<
                 endl;
        } else {
            cerr << "ERROR: " << error.code().message() << " " << error.path1() << endl;
        }

        retVal = error.code().value();
    }

    return retVal;
}

bool parse(const string &varIn, const path &pathIn, path *pathOut,
           const string &line = string(), int lineNum = 0)
{
    bool retVal = true;
    wordexp_t par;
    memset(&par, 0, sizeof(par));

    wordexp(varIn.c_str(), &par, WRDE_SHOWERR | WRDE_UNDEF);

    char **wp = par.we_wordv;

    for(size_t i = 0; i<par.we_wordc; ++i) {
        path current = pathIn;

        if(!is_directory(current)) {
            current = pathIn.parent_path();
        }

        if(fullPath(wp[i], current, pathOut)) {
            if(lineNum==0) {
                cerr << "INVALID FILE: " << varIn << endl;
            } else {
                cerr << "INVALID LINE: " << pathIn << ":" << lineNum << " => " << line << endl;
            }

            retVal = false;
        }
    }

    wordfree(&par);
    return retVal;
}

void evaluateDefine(const variables_map &vm, const string &var1,
                    const string &var2, const path &fp, const string &line, int lineNum)
{
    bool showLibs       = vm.count("libs");
    bool showCells      = vm.count("cells");
    bool showViews      = vm.count("views");
    bool showCellViews  = vm.count("cellviews");
    path libName(var1);
    path libPath;

    if(parse(var2, fp, &libPath, line, lineNum)) {
        cout << "DEFINE: " << fp.string() << ":" << lineNum << " \"" << line << "\""
             << endl;
        cout << "libPath: " << libPath.string() << endl;

        if(showLibs) {
            cout << "\tlibName: " << libName.string() << endl;
        }

        if(showCells || showViews || showCellViews) {
            recursive_directory_iterator it(libPath), eod;

            for(; it!=eod; ++it) {
                if(*it!=libPath) {
                    if(is_directory(it->path()) &&
                            it->path().parent_path()==libPath) {
                        if(showCells) {
                            cout << "\t\tcellName: " << it->path().filename().string() << endl;
                        }

                        if(showViews || showCellViews) {
                            recursive_directory_iterator it2(it->path()), eod2;

                            for(; it2!=eod2; ++it2) {
                                if(!is_directory(it2->path()) &&
                                        iCompareString(it2->path().extension().string(), string(".oa"))) {
                                    string fileName = it2->path().filename().string();
                                    string viewName = fileName.substr(0, fileName.size() - 3);

                                    if(showCellViews) {
                                        cout << "\t\tcellViewName: " << it->path().filename().string() << "/" <<
                                             viewName << endl;
                                    } else if(showViews) {
                                        cout << "\t\t\tviewName: " << viewName << endl;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void evaluate(const variables_map &vm, const string &libdefs);
void evaluateInclude(const variables_map &vm, const string &var1,
                     const path &fp, const string &line, int lineNum)
{
    bool showIncludes = vm.count("includes");
    path includePath;

    if(parse(var1, fp, &includePath, line, lineNum)) {
        if(fp!=includePath) {
            if(showIncludes) {
                cout << "INCLUDE: " << includePath.string() << endl;
            }

            evaluate(vm, includePath.string());
        } else {
            cerr << "ERROR: Recursion in file => " << fp << " includes file => " <<
                 includePath << " which references itself." << endl;
        }
    }
}

void readUntilEOF(stringstream *stream, string *outVar)
{
    string buffer;
    bool first = true;

    // To get the entire file path that might contain spaces.
    while(!stream->eof()) {
        if(!first) {
            (*outVar)+=" ";
        }

        (*stream) >> buffer;
        (*outVar)+=buffer;
        first = false;
    }
}

void evaluate(const variables_map &vm, const string &libdefs)
{
    bool showComments = vm.count("comments");
    path fp;

    try {
        if(parse(libdefs, current_path(), &fp)) {
            ifstream libdefsFile(fp.string().c_str());

            if(libdefsFile.is_open()) {
                int lineNum = 0;

                while(libdefsFile.good()) {
                    lineNum++;
                    string line;
                    getline(libdefsFile, line);

                    if(line.length()==0) {
                        continue;
                    }

                    string action;
                    stringstream stream(line);
                    stream >> action;

                    if(action[0]=='#') {
                        if(showComments) {
                            cout << "COMMENT: " << line << endl;
                        }

                        continue;
                    }

                    if(action=="DEFINE") {
                        string var1, var2;
                        stream >> var1;
                        readUntilEOF(&stream, &var2);

                        if(var1[0]=='#' || var2[0]=='#') {
                            cerr << "INVALID LINE: " << fp.string() << ":" << lineNum << " => " << line <<
                                endl;
                            continue;
                        }

                        if(var1.length()>0 && var2.length()>0) {
                            evaluateDefine(vm, var1, var2, fp, line, lineNum);
                        }
                    } else if(action=="INCLUDE") {
                        string var1;
                        readUntilEOF(&stream, &var1);

                        if(var1[0]=='#') {
                            cerr << "INVALID LINE: " << fp.string() << ":" << lineNum << " => " << line <<
                                endl;
                            continue;
                        }

                        if(var1.length()>0) {
                            evaluateInclude(vm, var1, fp, line, lineNum);
                        }
                    }
                }
            } else {
                cerr << "ERROR: Library path " << libdefs << " does not exist." << endl;
            }
        }
    } catch(...) {
        cerr << "ERROR: Library path " << fp << " does not exist." << endl;
    }
}

int main(int argc, char *argv[])
{
    options_description desc("Supported options.");
    desc.add_options()
    ("help",                            "Show this help message.")
    ("def" ,        value<string>(),    "Open this lib.defs file to parse.")
    ("libs",                            "Show lib names in the output.")
    ("cells",                           "Show cell names in the output.")
    ("views",                           "Show view names in the output.")
    ("cellviews",
     "Show cell/views as single line items in the output.")
    ("defines",                         "Show DEFINE statements in the output.")
    ("includes",                        "Show INCLUDE statements in the output.")
    ("comments",                        "Show comments in the output.")
    ("version,v",                       "Show version information.");

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if(vm.count("help")) {
        cout << desc << endl;
        return 0;
    }

    if(vm.count("version")){
        cout << "libdefsEval version 0.1.0" << endl;
        return 0;
    }

    string libdefs;

    if(vm.count("def")) {
        cout << "def: " << vm["def"].as<string>() << endl;
        libdefs = vm["def"].as<string>();
    } else {
        cerr << "ERROR: defs not defined." << endl;
        cerr << "   Try ./libdefsEval --defs lib.defs" << endl;
        return 1;
    }

    evaluate(vm, libdefs);

    return 0;
}
