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
 * 2017-06-05: Kevin Nesmith: Fixed the reading of the last parameter on a line.
 *                            There was an issue with multiple spaces.
 *
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
            break;
        }
    }

    wordfree(&par);
    return retVal;
}

void readUntilEOL(stringstream *stream, string *outVar)
{
    string buffer;

    getline(*stream, buffer);
    // getline does not trim the beginning of line.  There is usually whitespace.
    buffer.erase(buffer.begin(), find_if(buffer.begin(), buffer.end(),
                                         not1(ptr_fun<int, int>(isspace))));
    *outVar = buffer;
}

void evaluateDefine(const variables_map &vm, const string &var1,
                    const string &var2, const path &fp, const string &line, int lineNum)
{
    bool showLibs       = vm.count("libs");
    bool showCells      = vm.count("cells");
    bool showViews      = vm.count("views");
    bool showCellViews  = vm.count("cellviews");
    bool showDefines    = vm.count("defines");
    path libName(var1);
    path libPath;

    if(parse(var2, fp, &libPath, line, lineNum)) {
        if(showDefines) {
            cout << "DEFINE: " << fp.string() << ":" << lineNum << " \"" << line << "\"" <<
                 endl;
        }

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
                                    string viewName = it2->path().parent_path().filename().string();

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
                cout << "INCLUDE: " << fp.string() << ":" << lineNum << " " << includePath <<
                     endl;
            }

            evaluate(vm, includePath.string());
        } else {
            cerr << "ERROR: Recursion in file => " << fp <<
                 " includes file => " << includePath <<
                 " which references itself." << endl;
        }
    }
}

void evaluate(const variables_map &vm, const string &libdefs)
{
    bool showAssigns    = vm.count("assigns");
    bool showComments   = vm.count("comments");
    bool showEmptyLines = vm.count("emptylines");
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
                        if(showEmptyLines) {
                            cout << "EMPTY LINE: " << fp.string() << ":" <<
                                 lineNum << endl;
                        }

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
                        // var2 requires reading until EOL;
                        readUntilEOL(&stream, &var2);

                        if(var1[0]=='#' || var2[0]=='#') {
                            cerr << "INVALID LINE: " << fp.string() << ":" <<
                                 lineNum << " => " << line << endl;
                            continue;
                        }

                        if(var1.length()>0 && var2.length()>0) {
                            evaluateDefine(vm, var1, var2, fp, line, lineNum);
                        }
                    } else if(action=="INCLUDE") {
                        string var1;
                        // var1 requires reading until EOL
                        readUntilEOL(&stream, &var1);

                        if(var1[0]=='#') {
                            cerr << "INVALID LINE: " << fp.string() << ":" <<
                                 lineNum << " => " << line << endl;
                            continue;
                        }

                        if(var1.length()>0) {
                            evaluateInclude(vm, var1, fp, line, lineNum);
                        }
                    } else if(action=="ASSIGN") {
                        if(showAssigns) {
                            cout << "ASSIGN: " << fp.string() << ":" << lineNum << " \"" << line << "\"" <<
                                 endl;
                        }
                    } else {
                        cerr << "INVALID LINE: " << fp.string() << ":" <<
                             lineNum << " => " << line << endl;
                    }
                }
            } else {
                cerr << "ERROR: Library path " << libdefs <<
                     " does not exist." << endl;
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
    ("help,h",                          "Show this help message.")
    ("def,d",       value<string>(),    "Open this lib.defs file to parse.")
    ("libs",                            "Show lib names in the output.")
    ("cells",                           "Show cell names in the output.")
    ("views",                           "Show view names in the output.")
    ("cellviews",
     "Show cell/views as single line items in the output.")
    ("defines",                         "Show DEFINE statements in the output.")
    ("includes",                        "Show INCLUDE statements in the output.")
    ("assigns",                         "Show ASSIGN statements in the output.")
    ("comments",                        "Show COMMENTS in the output.")
    ("emptylines",                      "Show EMPTY LINES in the output.")
    ("version,v",                       "Show version information.");

    variables_map vm;
    store(parse_command_line(argc, argv, desc), vm);
    notify(vm);

    if(vm.count("help")) {
        cout << desc << endl;
        return 0;
    }

    if(vm.count("version")) {
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
