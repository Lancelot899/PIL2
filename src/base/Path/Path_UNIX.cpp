
#ifdef PLATEFORM_INCLUDE_SOURCE


#include <stdio.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>

#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include <climits>

#include "../Environment.h"

#include "../Utils/Ascii.h"
#include "../Debug/Assert.h"

#include "Path_UNIX.h"


namespace pi {


#ifndef PATH_MAX
#define PATH_MAX 1024 // fallback
#endif


std::string PathImpl::currentImpl()
{
    std::string path;
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)))
        path = cwd;
    else
        std::cerr<<("cannot get current directory");
//    std::string::size_type n = path.size();
//    if (n > 0 && path[n - 1] != '/') path.append("/");
    return path;
}


std::string PathImpl::homeImpl()
{
    std::string path;
    struct passwd* pwd = getpwuid(getuid());
    if (pwd)
        path = pwd->pw_dir;
    else
    {
        pwd = getpwuid(geteuid());
        if (pwd)
            path = pwd->pw_dir;
    }
    std::string::size_type n = path.size();
    if (n > 0 && path[n - 1] != '/') path.append("/");
    return path;
}


std::string PathImpl::tempImpl()
{
    std::string path;
    char* tmp = getenv("TMPDIR");
    if (tmp)
    {
        path = tmp;
        std::string::size_type n = path.size();
        if (n > 0 && path[n - 1] != '/') path.append("/");
    }
    else
    {
        path = "/tmp/";
    }
    return path;
}


std::string PathImpl::nullImpl()
{
#if defined(PIL_VXWORKS)
    return "/null";
#else
    return "/dev/null";
#endif
}


std::string PathImpl::expandImpl(const std::string& path)
{
    std::string result;
    std::string::const_iterator it  = path.begin();
    std::string::const_iterator end = path.end();
    if (it != end && *it == '~')
    {
        ++it;
        if (it != end && *it == '/')
        {
            result += homeImpl(); ++it;
        }
        else result += '~';
    }
    while (it != end)
    {
        if (*it == '$')
        {
            std::string var;
            ++it;
            if (it != end && *it == '{')
            {
                ++it;
                while (it != end && *it != '}') var += *it++;
                if (it != end) ++it;
            }
            else
            {
                while (it != end && (Ascii::isAlphaNumeric(*it) || *it == '_')) var += *it++;
            }
            char* val = getenv(var.c_str());
            if (val) result += val;
        }
        else result += *it++;
    }
    return result;
}


void PathImpl::listRootsImpl(std::vector<std::string>& roots)
{
    roots.clear();
    roots.push_back("/");
}



int  PathImpl::cmd(const std::string& commands)
{
    return system(commands.c_str());
}

bool PathImpl::pathExist(const std::string& path){

    struct stat     st;
    int             ret;

    ret = stat(path.c_str(), &st);
    return ret==0;
}

bool PathImpl::mkdir(const std::string& path)
{
    char            cmds[2048];
    sprintf(cmds, "mkdir -p '%s'", path.c_str());

    return system(cmds)==0;
}

bool PathImpl::rm(const std::string& path)
{
    char            cmd[2048];

    sprintf(cmd, "rm -rf '%s'", path.c_str());

    return system(cmd) == 0;
}

bool PathImpl::rename(const std::string& pOld, const std::string& pNew)
{
    char            cmd[2048];

    sprintf(cmd, "mv '%s' '%s'", pOld.c_str(), pNew.c_str());
    return system(cmd) == 0;
}

bool PathImpl::lsdir(const std::string& path,std::vector<std::string>& dl)
{
    DIR             *dir;
    struct dirent   *dp;

    // open directory
    dir = opendir(path.c_str());
    if( dir == NULL ) {
        pi_dbg_error("Failed to open dir: %s\n", dir_name.c_str());
        return false;
    }

    // get each items
    dl.clear();
    for(dp=readdir(dir); dp!=NULL; dp=readdir(dir)) {
        // skip .
        if( strlen(dp->d_name) == 1 && dp->d_name[0] == '.' )
            continue;

        // skip ..
        if( strlen(dp->d_name) == 2 && dp->d_name[0] == '.' && dp->d_name[1] == '.' )
            continue;

        // add to list
        dl.push_back(dp->d_name);
    }

    closedir(dir);

    // sort all file name
    std::sort(dl.begin(), dl.end());
    return true;
}

int PathImpl::is_file(const std::string& path)
{
    if( path.size() == 0 ) return -1;

    struct stat st;
    if (stat(path.c_str(), &st) == 0)
        return S_ISREG(st.st_mode);
    else
        return -1;

    return -1;
}

int PathImpl::is_dir(const std::string& path)
{
    if( path.size() == 0 ) return -1;

    struct stat st;
    if (stat(path.c_str(), &st) == 0)
        return S_ISDIR(st.st_mode);
    else
        return -1;

    return -1;
}

} // namespace pi


#endif // end of PLATEFORM_INCLUDE_SOURCE

