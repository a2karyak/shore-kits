
#include "engine/util/tmpfile.h"
#include "trace.h"
#include "qpipe_panic.h"



/**
 * @brief opens a temporary file with a unique name
 *
 * @param name string to store the new file's name in
 *
 * @return the file or NULL on error
 */
FILE *create_tmp_file(string &name, const string &prefix) {
    // TODO: use a configurable temp dir
    char meta_template[] = "tmp/%s.XXXXXX";
    int len = sizeof(meta_template) - 2 + prefix.size();
    char name_template[len + 1];
    sprintf(name_template, meta_template, prefix.c_str());
    int fd = mkstemp(name_template);
    if(fd < 0) {
        TRACE(TRACE_ALWAYS, "Unable to open temporary file %s!\n",
              name_template);
        QPIPE_PANIC();
    }
    TRACE(TRACE_TEMP_FILE, "Created temp file %s\n", name_template);


    // open a stream on the file
    FILE *file = fdopen(fd, "w");
    if(!file) {
        TRACE(TRACE_ALWAYS, "Unable to open a stream on %s\n",
              name_template);
        QPIPE_PANIC();
    }

    name = string(name_template);
    return file;
}

