#include <esp_log.h>
#include <esp_http_server.h>

#include "ui.h"

#define TAG "web_ui"

static httpd_handle_t server = NULL;
static bus_observer_t* bus_observer;

static esp_err_t
index_handler(httpd_req_t *req)
{
    httpd_resp_send(req, "<html><body><h1>It works!</h1></body></html>", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

static const httpd_uri_t index_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL,
};

static esp_err_t
memory_handler(httpd_req_t *req)
{
    uint8_t memory[1024];
    memcpy(memory, bus_observer->memory, PINBALL_MEMORY_LEN);

    httpd_resp_set_type(req, "application/octet-stream");
    httpd_resp_send(req, (const char*)memory, PINBALL_MEMORY_LEN);
    return ESP_OK;
}

static const httpd_uri_t memory_uri = {
    .uri = "/memory",
    .method = HTTP_GET,
    .handler = memory_handler,
    .user_ctx = NULL,
};

bool ui_init(bus_observer_t* bus_observer_)
{
    bus_observer = bus_observer_;

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &index_uri);
        httpd_register_uri_handler(server, &memory_uri);
        return true;
    } else {
        ESP_LOGI(TAG, "Error starting web UI!");
        return false;
    }
}