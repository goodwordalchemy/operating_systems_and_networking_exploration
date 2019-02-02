typedef struct __http_response_t {
    char *status_line;
    int status_code;
    char *content;
    int content_length;
} http_response_t;

void free_http_response(http_response_t *resp);

http_response_t *extract_response_content(char *response);
