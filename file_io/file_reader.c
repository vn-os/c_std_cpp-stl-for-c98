#include "file_reader.h"
#include "../string/string.h"
#include "../dir/dir.h"
#include "../fmt/fmt.h"
#include "../encoding/encoding.h"
#include <stdlib.h>

FileReader* file_reader_open(const char* filename, const ReadMode mode) {
    if (!filename) {
        fmt_fprintf(stderr, "Error: filename is null in file_reader_open.\n");
        return NULL;
    }

    FileReader* reader = (FileReader*) malloc(sizeof(FileReader));
    if (!reader) {
        fmt_fprintf(stderr, "Error: Can not allocate memory for FileReader in file_reader_open.\n");
        return NULL;
    }
    const char* modeStr = NULL;

    switch (mode){
    case READ_TEXT:
        modeStr = "r";
        break;
    case READ_BINARY:
        modeStr = "rb";
        break;
    case READ_UNICODE:
        #if defined(_WIN32) || defined(_WIN64)
        modeStr = "r, ccs=UTF-16LE";
        #else
        modeStr = "r";
        #endif 
        break;
    case READ_BUFFERED:
        modeStr = "r";
        break;
    case READ_UNBUFFERED:
        modeStr = "r";
        break;
    default:
        fmt_fprintf(stdout, "Warning: Not Valid mode for reading in file_reader_open i initialize default mode that is 'r'.\n");
        #if defined(_WIN32) || defined(_WIN64)
        modeStr = "r, ccs=UTF-16LE";
        #else
        modeStr = "r";
        #endif 
        break;
    }

    #if defined(_WIN32) || defined(_WIN64)
    wchar_t* wFileName = encoding_utf8_to_wchar(filename);
    wchar_t* wMode = encoding_utf8_to_wchar(modeStr);

    if (!wMode) {
        fmt_fprintf(stderr, "Error: Can not convert mode to wchar in file_reader_open.\n");
        return NULL;
    }
    if (!wFileName) {
        fmt_fprintf(stderr, "Error: Can not convert filename to wchar in file_reader_open.\n");
        return NULL;
    }
    reader->file_reader = _wfopen(wFileName, wMode);
    free(wMode);
    free(wFileName);
    #else 
    reader->file_reader = fopen(filename, modeStr);
    #endif 

    if (reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: Can not open file in file_reader_open.\n");
        free(reader);
        return NULL;
    }
    reader->mode = mode;
    reader->is_open = true;
    reader->encoding = READ_ENCODING_UTF16;
    reader->file_path = string_strdup(dir_absolute_file_path(filename));
    return reader;
}

bool file_reader_close(FileReader *reader) {
    if (!reader) {
        fmt_fprintf(stderr, "Error: FileReader object is null in file_reader_close.\n");
        return false;
    }
    if (reader->file_reader && fclose(reader->file_reader)) {
        fmt_fprintf(stderr, "Error: Failed to close file in file_reader_close.\n");
        return false;
    }
    if (reader->file_path) {
        free(reader->file_path);
        reader->file_path = NULL;
    }

    reader->is_open = false;
    free(reader); // Optionally free the FileReader object itself if it's dynamically allocated
    return true;
}

size_t file_reader_get_position(FileReader *reader) {
    if (reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: FileReader object is null and not valid in file_reader_get_position.\n");
        return (size_t)-1;
    }

    long cursor_position = ftell(reader->file_reader);
    if (cursor_position == -1L) {
        fmt_fprintf(stderr, "Error: Could not determine file position.\n");
        return (size_t)-1;
    }
    return (size_t)cursor_position;
}

bool file_reader_is_open(FileReader* reader) {
    if (!reader) {
        fmt_fprintf(stderr, "Error: FileReader pointer is NULL in file_reader_is_open.\n");
        return false;
    }
    if (reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: FileReader object is NULL and its not open in file_reader_is_open.\n");
        return false;
    }
    return reader->is_open;
}

bool file_reader_set_encoding(FileReader* reader, const ReadEncodingType encoding) {
    if (!reader || reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: FileReader object is invalid or NULL in file_reader_set_encoding.\n");
        return false;
    }
    if (!(encoding >= READ_ENCODING_UTF16 && encoding <= READ_ENCODING_UTF32)) {
        fmt_fprintf(stderr, "Error: Encoding type is Invalid in file_reader_set_encoding.\n");
        return false;
    }
    reader->encoding = encoding;
    return true;
}

const char *file_reader_get_file_name(FileReader *reader){
    if (!reader || reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: FileReader object is null and not valid in file_reader_get_file_name.\n");
        return NULL;
    }
    if (!reader->file_path) {
        fmt_fprintf(stderr, "Error: file path for FileReader is null in file_reader_get_file_name.\n");
        return NULL;
    }
    return (const char*)reader->file_path;
}

// Move the file pointer to a specific location for random access writing
bool file_reader_seek(FileReader *reader, long offset, const CursorPosition cursor_pos) {
    if (!reader || reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: FileReader object is null and invalid in file_reader_seek.\n");
        return false;
    }
    int pos;

    switch (cursor_pos) {
        case POS_BEGIN:
            pos = SEEK_SET;
            break;
        case POS_END:
            pos = SEEK_END;
            break;
        case POS_CURRENT:
            pos = SEEK_CUR;
            break;
        default:
            fmt_fprintf(stderr,"Warning: Cursor position is Invalid defailt pos is POS_BEGIN in file_reader_seek.\n");
            pos = SEEK_SET;
            break;
    }
    
    if (fseek(reader->file_reader, offset, pos) != 0) {
        fmt_fprintf(stderr, "Error: fseek failed in file_reader_seek.\n");
        return false;
    }
    return true;
}

bool file_reader_eof(FileReader* reader) {
    if (!reader || reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: FileReader object is NULL and invalid in file_reader_eof.\n");
        return false;
    }
    return feof(reader->file_reader) != 0;
}

size_t file_reader_get_size(FileReader* reader) {
    if (!reader || reader->file_reader == NULL) {
        fmt_fprintf(stderr, "Error: FileReader object is not valid and NULL in file_reader_get_size.\n");
        return 0;
    }
    long current_position = file_reader_get_position(reader);
    if (fseek(reader->file_reader, 0, SEEK_END) != 0) {
        fmt_fprintf(stderr, "Error: fseek failed to seek to end of file in file_reader_get_size.\n");
        return 0;
    }

    size_t size = file_reader_get_position(reader);
    if (fseek(reader->file_reader, current_position, SEEK_SET) != 0) {
        fmt_fprintf(stderr, "Error: fseek failed to return to original position in file_reader_get_size.\n");
    }
    return size;
}

size_t file_reader_read(void* buffer, size_t size, size_t count, FileReader* reader) {
    if (!reader || !reader->file_reader || !buffer) {
        fmt_fprintf(stderr, "Error: Invalid argument in file_reader_read.\n");
        return 0;
    }
    (void)size;
    // Allocate a temporary buffer for reading raw UTF-16 data
    wchar_t* rawBuffer = malloc(sizeof(wchar_t) * (count + 1));  // +1 for potential null-termination
    if (!rawBuffer) {
        fmt_fprintf(stderr, "Error: Memory allocation failed in file_reader_read.\n");
        return 0;
    }

    // Read UTF-16 data from the file
    size_t actualRead = fread(rawBuffer, sizeof(wchar_t), count, reader->file_reader);
    // Null-terminate the raw buffer if your conversion function requires it
    rawBuffer[actualRead] = L'\0';

    size_t convertedSize = 0;

    switch (reader->encoding) {
        case READ_ENCODING_UTF16: {
            // Convert UTF-16 to UTF-8 using your conversion function
            char* utf8Buffer = encoding_wchar_to_utf8(rawBuffer);

            if (utf8Buffer) {
                size_t bytesToCopy = string_length_utf8(utf8Buffer);
                
                if (bytesToCopy > count - 1) {
                    bytesToCopy = count - 1;  // Limit to the size of the output buffer
                }
                strcpy(buffer, utf8Buffer);
                convertedSize = bytesToCopy;
                free(utf8Buffer);
            } 
            else {
                fmt_fprintf(stderr, "Error: Conversion to UTF-8 failed in file_reader_read.\n");
            }
            break;
        }
        // Add cases for other encodings if needed
        default:
            fmt_fprintf(stderr, "Unsupported encoding in file_reader_read.\n");
            break;
    }

    free(rawBuffer);
    return convertedSize;
}

bool file_reader_read_line(char* buffer, size_t size, FileReader* reader) {
    if (!reader || !reader->file_reader || !buffer) {
        fmt_fprintf(stderr, "Error: Invalid argument in file_reader_read_line.\n");
        return false;
    }

    wchar_t wBuffer[1024];  // Temporary buffer for reading UTF-16 data
    char* utf8Buffer = NULL;
    bool done = false;
    size_t totalRead = 0;   // Total bytes read in UTF-8

    while (!done) {
        wchar_t wc = fgetwc(reader->file_reader);
        if (wc == WEOF || wc == L'\n' || totalRead >= size - 1) {
            done = true;
        } else {
            // Append wchar to the temporary buffer
            wBuffer[totalRead++] = wc;
        }
    }
    wBuffer[totalRead] = L'\0';  // Null-terminate the wchar buffer

    // Convert the wchar buffer to UTF-8 if necessary
    switch (reader->encoding) {
        case READ_ENCODING_UTF16:
            utf8Buffer = encoding_wchar_to_utf8(wBuffer);
            if (!utf8Buffer) {
                fmt_fprintf(stderr, "Error: Conversion to UTF-8 failed in file_reader_read_line.\n");
                return false;
            }
            strncpy(buffer, utf8Buffer, size - 1);
            buffer[size - 1] = '\0';  // Ensure null-termination
            free(utf8Buffer);
            break;

        default:
            // If no conversion is needed, just copy the data
            wcstombs(buffer, wBuffer, size);
            break;
    }
    return totalRead > 0;
}

size_t file_reader_read_fmt(FileReader* reader, const char* format, ...) {
    if (!reader || !reader->file_reader || !format) {
        fmt_fprintf(stderr, "Error: Invalid argument in file_reader_read_fmt.\n");
        return 0;
    }

    wchar_t wBuffer[1024]; // Wide character buffer for UTF-16 data
    if (fgetws(wBuffer, sizeof(wBuffer) / sizeof(wchar_t), reader->file_reader) == NULL) {
        return 0; // No data read or error occurred
    }

    // Convert UTF-16 to UTF-8
    char* utf8Buffer = encoding_wchar_to_utf8(wBuffer);
    if (!utf8Buffer) {
        fmt_fprintf(stderr, "Error: UTF-16 to UTF-8 conversion failed in file_reader_read_fmt.\n");
        return 0;
    }

    va_list args;
    va_start(args, format);

    size_t read = vsscanf(utf8Buffer, format, args);

    va_end(args);

    free(utf8Buffer);
    return read; // Number of items successfully read
}

bool file_reader_copy(FileReader* src_reader, FileWriter* dest_writer) {
    if (!src_reader || !src_reader->file_reader || !dest_writer || !dest_writer->file_writer) {
        fmt_fprintf(stderr, "Error: Invalid argument in file_reader_copy.\n");
        return false;
    }

    wchar_t wBuffer[1024]; // Wide character buffer for UTF-16 data
    size_t bytesRead, bytesToWrite;

    while ((bytesRead = fread(wBuffer, sizeof(wchar_t), 1024, src_reader->file_reader)) > 0) {
        char* utf8Buffer = NULL;
        size_t utf8BufferSize = 0;

        // Convert the read data to UTF-8 if necessary
        switch (src_reader->encoding) {
            case READ_ENCODING_UTF16:
                utf8Buffer = encoding_wchar_to_utf8(wBuffer);
                if (!utf8Buffer) {
                    fmt_fprintf(stderr, "Error: Conversion to UTF-8 failed in file_reader_copy.\n");
                    return false;
                }
                utf8BufferSize = string_length_utf8(utf8Buffer);
                break;

            default:
                fmt_fprintf(stderr, "Unsupported encoding in file_reader_copy.\n");
                return false;
        }

        // Write the UTF-8 data to the destination file
        bytesToWrite = utf8BufferSize;
        size_t bytesWritten = file_writer_write(utf8Buffer, sizeof(char), bytesToWrite, dest_writer);
        free(utf8Buffer);

        if (bytesWritten < bytesToWrite) {
            fmt_fprintf(stderr, "Error: Could not write all data in file_reader_copy.\n");
            return false;
        }
    }
    return true;
}

bool file_reader_read_lines(FileReader* reader, char*** buffer, size_t num_lines) {
    if (!reader || !reader->file_reader || !buffer) {
        fmt_fprintf(stderr, "Error: Invalid arguments in file_reader_read_lines.\n");
        return false;
    }

    *buffer = malloc(num_lines * sizeof(char*));
    if (!*buffer) {
        fmt_fprintf(stderr, "Error: Memory allocation failed in file_reader_read_lines.\n");
        return false;
    }

    size_t lines_read = 0;
    char line_buffer[1024]; // Adjust the size as needed

    while (lines_read < num_lines && !feof(reader->file_reader)) {
        if (file_reader_read_line(line_buffer, sizeof(line_buffer), reader)) {
            (*buffer)[lines_read] = strdup(line_buffer);
            if (!(*buffer)[lines_read]) {
                fmt_fprintf(stderr, "Error: Memory allocation failed for line in file_reader_read_lines.\n");
                // Free previously allocated lines
                for (size_t i = 0; i < lines_read; ++i) {
                    free((*buffer)[i]);
                }
                free(*buffer);
                return false;
            }
            lines_read++;
        }
    }
    return lines_read == num_lines;
}
