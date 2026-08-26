/*
 * CS 5502-71 Real Time Operating Systems
 *
 * Author: Jacob A. Geldbach
 * Final
 * server.c
 */

#include "common.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include <netinet/tcp.h>
#include <sys/param.h>
#include <sys/socket.h>

#include "driver/gpio.h"

/* Shared latest_radar struct and mutex */
#include "radar.h"
#include "wifi_secrets.h"

static const char server_task_tag[40] = "Web server task";
static const char stream_radar_task_tag[40] = "Stream Radar Data Task";
static bool wifi_connected = false;

/* Mutex for sse data stream clients */
static SemaphoreHandle_t sse_mutex;

/* SSE client tracking */
#define MAX_SSE_CLIENTS 4
static int sse_clients[MAX_SSE_CLIENTS] = {-1, -1, -1, -1};

/* Event handler for the wifi driver, this should cover all the connect/disconnect events we care about */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                              int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(server_task_tag, "WiFi started, connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = event_data;
        ESP_LOGI(server_task_tag, "Disconnected! reason=%d, retrying...", event->reason);
        esp_wifi_connect();
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(server_task_tag, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));

        wifi_connected = true;
    }
}

/* Add a client socket to SSE broadcast list */
static bool sse_add_client(int sock)
{
    bool added = false;
    xSemaphoreTake(sse_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sse_clients[i] == -1) {
            sse_clients[i] = sock;
            added = true;
            ESP_LOGI(stream_radar_task_tag, "SSE client added at slot %d", i);
            break;
        }
    }
    xSemaphoreGive(sse_mutex);
    return added;
}

/* Remove a client socket from SSE broadcast list */
static void sse_remove_client(int sock)
{
    xSemaphoreTake(sse_mutex, portMAX_DELAY);
    for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
        if (sse_clients[i] == sock) {
            sse_clients[i] = -1;
            ESP_LOGI(stream_radar_task_tag, "SSE client removed from slot %d", i);
            break;
        }
    }
    xSemaphoreGive(sse_mutex);
}

void stream_radar_init(void)
{
    sse_mutex = xSemaphoreCreateMutex();
}

void v_stream_radar_data_task(void* pv_parameters)
{
    ESP_LOGI(stream_radar_task_tag, "SSE Broadcaster task started");
     
    while (1) {

        radar_data_t copy;
        /* Take the radar mutex, quickly copy the latest_radar shared struct into a copy on this tasks stack 
         * then give the mutex back to the faster running processes (stepper and radar) */
        xSemaphoreTake(radar_mutex, portMAX_DELAY);
        copy = latest_radar;
        latest_radar.in_range = false;
        xSemaphoreGive(radar_mutex);


        /* This is so confusing, but because I want the radar to go left to right (clockwise) on the screen
         * and the stepper motor due to the way its built needs to have its poles powered in a away that the count_halfsteps
         * has to be subtracted, this needs to be inverted */
        copy.current_angle = 120.0f - copy.current_angle;

        /* Build SSE message based on how the HTML and JS page we reponded with originally wants the data */
        char msg[256];
        snprintf(msg, sizeof(msg),
            "data: {\"angle\":%.1f,\"distance\":%.2f,\"valid\":%s}\n\n",
            copy.current_angle,
            copy.target_distance,
            copy.in_range ? "true" : "false"
        );

        /* Broadcast to all connected SSE clients */
        xSemaphoreTake(sse_mutex, portMAX_DELAY);
        for (int i = 0; i < MAX_SSE_CLIENTS; i++) {
            if (sse_clients[i] != -1) {
                int sent = send(sse_clients[i], msg, strlen(msg), MSG_NOSIGNAL);
                if (sent < 0) {
                    ESP_LOGI(stream_radar_task_tag, "SSE client %d disconnected, removing", i);
                    close(sse_clients[i]);
                    sse_clients[i] = -1;
                }
            }
        }
        xSemaphoreGive(sse_mutex);

        vTaskDelay(pdMS_TO_TICKS(50));
    }
}


void v_web_server(void* pv_parameters)
{
    ESP_LOGI(server_task_tag, "Starting Web server task");
    
    /* Init wifi connection */
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();

    /* Register the event handlers for wifi connection/disconnection and IP assignment */
    esp_event_handler_instance_register(
        WIFI_EVENT,
        ESP_EVENT_ANY_ID,
        &wifi_event_handler,
        NULL,
        NULL
    );

    esp_event_handler_instance_register(
        IP_EVENT,
        IP_EVENT_STA_GOT_IP,
        &wifi_event_handler,
        NULL,
        NULL
    );

    esp_netif_create_default_wifi_sta();

    /* Init esp wifi driver with defaults */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    /* Specific to target WIFI */
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    
    /* Apply target Wifi config */
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);

    /* Start ESP Wifi driver our event handler will handle the initial connect */
    esp_wifi_start();
    
    /* Wait until we connect to the wifi */
    while(!wifi_connected) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* Initial configuration for LED GPIO XXX remove when switch to radar*/
    gpio_reset_pin(GPIO_NUM_13);
    gpio_set_direction(GPIO_NUM_13, GPIO_MODE_INPUT_OUTPUT);
    
    ESP_LOGI(server_task_tag, "WiFi ready, starting web server");

    /* Set up socket for initail connection and response */
    int server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (server_fd < 0) {
        ESP_LOGE(server_task_tag, "Failed to create socket");
        vTaskDelete(NULL);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr = {
        .sin_family = AF_INET,
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_port = htons(80),
    };

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        ESP_LOGE(server_task_tag, "Failed to bind");
        close(server_fd);
        vTaskDelete(NULL);
    }

    listen(server_fd, 5);
    ESP_LOGI(server_task_tag, "Server listening on port 80");

    /* Giant mess of HTML string since esp32 doesn't have a file system, this will be the response everyone who connects gets,
     * mostly just javascript that draws the radar pickture based on the last value of the sse message payload that the broadcast task sent out */
    const char* html_response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n"
        "Connection: close\r\n"
        "\r\n"
        "<!DOCTYPE html><html><head>"
        "<meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>Stepper Radar</title>"
        "<style>"
        "html,body{margin:0;padding:0;background:#000;overflow:hidden;}"
        "canvas{display:block;}"
        "#status{position:absolute;top:10px;left:10px;color:#0f0;font-family:monospace;font-size:16px;}"
        "</style>"
        "</head><body>"
        "<canvas id='radar'></canvas>"
        "<div id='status'>Connecting...</div>"
            "<script>"
                "var canvas=document.getElementById('radar');"
                "var ctx=canvas.getContext('2d');"
                "var status=document.getElementById('status');"
                "var w,h,cx,cy,maxR;"
                "var MAX_FEET=5;"
                "var targets=[];"
                "var sweepAngle=0;"

                "function resize(){"
                    "canvas.width=window.innerWidth;"
                    "canvas.height=window.innerHeight;"
                    "w=canvas.width;"
                    "h=canvas.height;"
                    "cx=w/2;"
                    "cy=h*0.95;"
                    "maxR=Math.min(w*0.48,h*0.85);"
                "}"
                
                "window.addEventListener('resize',resize);"
                "resize();"

                "function radarToCanvasAngle(deg){"
                    "return (210 + deg) * Math.PI / 180;"
                "}"

                "function drawGrid(){"
                    "ctx.strokeStyle='#0f0';"
                    "ctx.fillStyle='#0f0';"
                    "ctx.lineWidth=1;"
                    "ctx.font='14px monospace';"

                    "for(var i=1;i<=5;i++){"
                        "var r=(i/5)*maxR;"
                        "ctx.beginPath();"
                        "ctx.arc(cx,cy,r,radarToCanvasAngle(0),radarToCanvasAngle(120),false);"
                        "ctx.stroke();"
                        "ctx.fillText(i+' ft',cx+8,cy-r+14);"
                    "}"

                    "var leftA=radarToCanvasAngle(0);"
                    "var rightA=radarToCanvasAngle(120);"

                    "ctx.beginPath();"
                    "ctx.moveTo(cx,cy);"
                    "ctx.lineTo(cx+Math.cos(leftA)*maxR,cy+Math.sin(leftA)*maxR);"
                    "ctx.stroke();"

                    "ctx.beginPath();"
                    "ctx.moveTo(cx,cy);"
                    "ctx.lineTo(cx+Math.cos(rightA)*maxR,cy+Math.sin(rightA)*maxR);"
                    "ctx.stroke();"
                "}"

                "function drawSweep(){"
                    "var ca=radarToCanvasAngle(sweepAngle);"
                    "var x=cx+Math.cos(ca)*maxR;"
                    "var y=cy+Math.sin(ca)*maxR;"
                    "ctx.strokeStyle='rgba(0,255,0,0.9)';"
                    "ctx.lineWidth=3;"
                    "ctx.beginPath();"
                    "ctx.moveTo(cx,cy);"
                    "ctx.lineTo(x,y);"
                    "ctx.stroke();"
                "}"

                "function drawTargets(){"
                    "var now=Date.now();"
                    "targets=targets.filter(function(t){return now-t.time<4000;});"

                    "for(var i=0;i<targets.length;i++){"
                        "var t=targets[i];"
                        "var age=now-t.time;"
                        "var alpha=1-(age/4000);"
                        "var ca=radarToCanvasAngle(t.angle);"
                        "var r=(t.distance/MAX_FEET)*maxR;"
                        
                        "if(r>maxR)r=maxR;"
                            "var x=cx+Math.cos(ca)*r;"
                            "var y=cy+Math.sin(ca)*r;"
                            "ctx.fillStyle='rgba(0,255,0,'+alpha+')';"
                            "ctx.beginPath();"
                            "ctx.arc(x,y,6,0,Math.PI*2);"
                            "ctx.fill();"
                        "}"
                    "}"

                "function draw(){"
                    "ctx.fillStyle='rgba(0,0,0,0.22)';"
                    "ctx.fillRect(0,0,w,h);"
                    "drawGrid();"
                    "drawTargets();"
                    "drawSweep();"
                    "requestAnimationFrame(draw);"
                "}"
        
                "draw();"

                "function connect(){"
                    "var es=new EventSource('/stream');"
                    "es.onopen=function(){"
                    "status.innerText='LIVE';"
                "};"

                "es.onerror=function(){"
                    "status.innerText='RECONNECTING...';"
                    "es.close();"
                    "setTimeout(connect,2000);"
                "};"

                "es.onmessage=function(e){"
                "try{"
                    "var d=JSON.parse(e.data);"
                    "var angle=Number(d.angle);"
                    "var dist=Number(d.distance);"
                    "var valid=(d.valid===true);"

                    "if(isNaN(angle)||isNaN(dist))return;"
                    "if(angle<0)angle=0;"
                    "if(angle>120)angle=120;"
                    "if(dist<0)dist=0;"

                    "sweepAngle=angle;"

                    "if(valid&&dist>0&&dist<=MAX_FEET){"
                    "targets.push({angle:angle,distance:dist,time:Date.now()});"
                "}"
                "}catch(err){"
                    "console.log(err);"
                "}"
            "};"
        "}"

        "connect();"
        "</script>"
        "</body></html>";

    const char* sse_headers = 
        "HTTP/1.1 200 OK\r\n" "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n" "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n" "\r\n";

    /* Once a client connects to the web server running on the ip address we were assigned from the Wifi, add the client socket_fd to the sse_list
     * the sse_list is served a continuous stream of data via the radar stream task
     * allow for up to 5 broadcast clients that will be able to see the stream of radar data */
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);

        int client_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            ESP_LOGE(server_task_tag, "Failed to accept connection");
            continue;
        }

        char rx_buffer[512];
        int len = recv(client_sock, rx_buffer, sizeof(rx_buffer) - 1, 0);

        if (len <= 0) {
            close(client_sock);
            continue;
        }
   
        rx_buffer[len] = '\0';
        ESP_LOGI(server_task_tag, "REQUEST:\n%s", rx_buffer);
        /* Handle SSE stream request - add client to broadcast list */
        if (strstr(rx_buffer, "GET /stream")) {
            ESP_LOGI(server_task_tag, "New SSE client requesting stream");
           
            if (sse_add_client(client_sock)) {
                /* Send SSE headers, then broadcaster task takes over */
                int flag = 1;
                setsockopt(client_sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
                send(client_sock, sse_headers, strlen(sse_headers), 0);
                /* Don't close socket - broadcaster owns it now */
            } else {
                ESP_LOGI(server_task_tag, "Max SSE clients reached, rejecting");
                const char* err = "HTTP/1.1 503 Service Unavailable\r\n\r\n";
                send(client_sock, err, strlen(err), 0);
                close(client_sock);
            }
            continue;
        }
        else { /* Serve the HTML page, this is when clients first connect */
            send(client_sock, html_response, strlen(html_response), 0);
        }
       
        close(client_sock);
    }
}

