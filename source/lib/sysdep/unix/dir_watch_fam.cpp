#include "precompiled.h"

#include <map>
#include <string>

#include "lib.h"
#include "res/file.h"

#include <fam.h>

static FAMConnection fc;
static bool initialized;

static std::map<intptr_t, std::string> dirs;

// path: portable and relative, must add current directory and convert to native
// better to use a cached string from rel_chdir - secure
int dir_add_watch(const char* const path, intptr_t* const watch)
{
	if(!initialized)
	{
		CHECK_ERR(FAMOpen2(&fc, "lib_res"));
		atexit2((void*)FAMClose, (uintptr_t)&fc);
		initialized = true;
	}

	char n_full_path[PATH_MAX];
	CHECK_ERR(file_make_native_path(path, n_full_path));

	FAMRequest req;
	if(FAMMonitorDirectory(&fc, n_full_path, &req, (void*)0) < 0)
	{
		*watch = -1;
		debug_warn("res_watch_dir failed!");
		return -1;	// no way of getting error code?
	}

	*watch = (intptr_t)req.reqnum;
	dirs[*watch] = n_full_path;
	return 0;
}


int dir_cancel_watch(const intptr_t watch)
{
	if(!initialized)
	{
		debug_warn("dir_cancel_watch before dir_add_watch");
		return -1;
	}

	FAMRequest req;
	req.reqnum = (int)watch;
	return FAMCancelMonitor(&fc, &req);
}

int dir_get_changed_file(char* fn)
{
	if(!initialized)
		return -1;

	FAMEvent e;
	while(FAMPending(&fc) > 0)
		if(FAMNextEvent(&fc, &e) >= 0)
		{
			char n_path[PATH_MAX];
			const char* dir = dirs[e.fr.reqnum].c_str();
			snprintf(n_path, PATH_MAX, "%s%c%s", dir, DIR_SEP, e.filename);
			CHECK_ERR(file_make_portable_path(n_path, fn));
			return 0;
		}

	return 1;
}
