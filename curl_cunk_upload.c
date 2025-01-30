#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define CHUNK_SIZE (1024 * 16) // 16KB chunks

struct ChunkData {
    FILE *file;
    size_t total_size;
    size_t offset;
};

static size_t read_callback(char *buffer, size_t size, size_t nitems, void *userdata) {
    struct ChunkData *chunk = (struct ChunkData *)userdata;
    size_t bytes_to_read = size * nitems;
    size_t bytes_available = chunk->total_size - chunk->offset;

    if (bytes_available == 0) {
        return 0; // Signal end of file
    }

    size_t bytes_to_copy = (bytes_to_read < bytes_available) ? bytes_to_read : bytes_available;

    size_t bytes_read = fread(buffer, 1, bytes_to_copy, chunk->file);
    if (bytes_read != bytes_to_copy) {
        // Handle file read error (e.g., log it)
        fprintf(stderr, "Error reading from file.\n");
        return 0; // Or return bytes_read if you want to continue
    }

    chunk->offset += bytes_read;
    return bytes_read;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *file = fopen(filename, "rb"); // Open in binary mode
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    size_t file_size = ftell(file);
    fseek(file, 0, SEEK_SET); // Rewind to the beginning

    struct ChunkData chunk;
    chunk.file = file;
    chunk.total_size = file_size;
    chunk.offset = 0;

    CURL *curl;
    CURLcode res;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://your-server-address/upload");

        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &chunk);

        curl_easy_setopt(curl, CURLOPT_TRANSFER_ENCODING, "chunked");
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)file_size);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }

        curl_easy_cleanup(curl);
    } else {
        fprintf(stderr, "curl_easy_init() failed\n");
    }

    fclose(file);
    curl_global_cleanup();

    return 0;
}
