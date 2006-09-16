
#include <cstdlib>
#include "util/tmpfile.h"
#include "util/exception.h"
#include "util/trace.h"



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
    char name_template[len + 1];
    sprintf(name_template, meta_template, prefix.data());
    int fd = mkstemp(name_template);
    if(fd < 0)
        throw EXCEPTION(FileException,
                        "Caught %s while opening temp file %s",
                        errno_to_str().data(), name_template);
    
    TRACE(TRACE_TEMP_FILE, "Created temp file %s\n", name_template);


    // open a stream on the file
    FILE *file = fdopen(fd, "w");
    if(!file)
        throw EXCEPTION(FileException,
                        "Caught %s while opening a stream on %s",
                        errno_to_str().data(), name_template);

    name = name_template;
    return file;
}

