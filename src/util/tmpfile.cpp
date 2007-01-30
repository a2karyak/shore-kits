
#include <stdlib.h>
#include "util/tmpfile.h"
#include "util/exception.h"
#include "util/trace.h"
#include "util/guard.h"
#include <stdio.h>

/**
 * @brief opens a temporary file with a unique name
 *
 * @param name string to store the new file's name in
 *
 * @return the file or NULL on error
 */
FILE *create_tmp_file(c_str &name, const c_str &prefix) {
    // TODO: use a configurable temp dir
    char meta_template[] = "tmp/%s.XXXXXX";
    int len = sizeof(meta_template) - 2 + strlen(prefix);
    array_guard_t<char> name_template = new char[len + 1];
    sprintf(name_template, meta_template, prefix.data());
    int fd = mkstemp(name_template);
    if(fd < 0)
        THROW3(FileException,
                        "Caught %s while opening temp file %s",
                        errno_to_str().data(), (char*) name_template);
    
    TRACE(TRACE_TEMP_FILE, "Created temp file %s\n", (char*) name_template);


    // open a stream on the file
    FILE *file = fdopen(fd, "w");
    if(!file)
        THROW3(FileException,
                        "Caught %s while opening a stream on %s",
                        errno_to_str().data(), (char*) name_template);

    name = (char*) name_template;
    return file;
}

