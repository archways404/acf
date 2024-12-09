#include <curl/curl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CONFIG_FILE ".afcconfig"

// Function to get the home directory
char* get_home_directory() {
  struct passwd* pw = getpwuid(getuid());
  return pw->pw_dir;
}

// Function to write the URL to the config file
void save_config(const char* url) {
  char config_path[1024];
  snprintf(config_path, sizeof(config_path), "%s/%s", get_home_directory(), CONFIG_FILE);

  FILE* config_file = fopen(config_path, "w");
  if (!config_file) {
    perror("Failed to open config file");
    exit(EXIT_FAILURE);
  }
  fprintf(config_file, "%s\n", url);
  fclose(config_file);
  printf("URL saved to config file: %s\n", config_path);
}

// Function to read the URL from the config file
char* read_config() {
  static char url[2048];
  char        config_path[1024];
  snprintf(config_path, sizeof(config_path), "%s/%s", get_home_directory(), CONFIG_FILE);

  FILE* config_file = fopen(config_path, "r");
  if (!config_file) {
    fprintf(stderr, "Config file not found. Please specify a URL.\n");
    exit(EXIT_FAILURE);
  }
  if (fgets(url, sizeof(url), config_file) == NULL) {
    fprintf(stderr, "Failed to read URL from config file.\n");
    fclose(config_file);
    exit(EXIT_FAILURE);
  }
  fclose(config_file);
  // Remove newline character
  url[strcspn(url, "\n")] = '\0';
  return url;
}

// Callback function for libcurl to write data
size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
  size_t total_size = size * nmemb;
  FILE*  fp = (FILE*)userp;
  fwrite(contents, size, nmemb, fp);
  return total_size;
}

// Function to fetch the file from the URL
void fetch_and_create(const char* url) {
  CURL* curl = curl_easy_init();
  if (!curl) {
    fprintf(stderr, "Failed to initialize CURL.\n");
    exit(EXIT_FAILURE);
  }

  FILE* file = fopen(".clang-format", "wb");
  if (!file) {
    perror("Failed to create .clang-format");
    curl_easy_cleanup(curl);
    exit(EXIT_FAILURE);
  }

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);

  CURLcode res = curl_easy_perform(curl);
  if (res != CURLE_OK) {
    fprintf(stderr, "CURL error: %s\n", curl_easy_strerror(res));
  } else {
    printf(".clang-format created successfully.\n");
  }

  fclose(file);
  curl_easy_cleanup(curl);
}

int main(int argc, char* argv[]) {
  if (argc == 2) {
    // Save the URL to the config file
    save_config(argv[1]);
  } else if (argc == 1) {
    // Read the URL from the config file and fetch the file
    char* url = read_config();
    fetch_and_create(url);
  } else {
    fprintf(stderr, "Usage: %s [url]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  return 0;
}
