#ifdef ENABLE_WEB_SERVER
#include "globals.h"
#include <WebServer.h>
#include "ui.h"

WebServer server(80);
File uploadFile;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>M5Cardputer File Manager</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: sans-serif; background-color: #282c34; color: #abb2bf; }
    h2, h3 { color: #61afef; border-bottom: 1px solid #444; padding-bottom: 5px; }
    table { width: 100%; border-collapse: collapse; margin-bottom: 20px;}
    th, td { padding: 8px; text-align: left; border-bottom: 1px solid #444; }
    tr:hover { background-color: #3e4451; }
    .btn-delete { background-color: #e06c75; color: white; border: none; padding: 5px 10px; cursor: pointer; }
    input[type="file"] { margin-top: 15px; }
    input[type="submit"] { background-color: #98c379; color: white; border: none; padding: 10px 20px; cursor: pointer; }
    #sd-status { display: flex; justify-content: space-around; background-color: #3e4451; padding: 10px; border-radius: 5px; margin-bottom: 20px; }
    #sd-status div { text-align: center; }
    #sd-status .label { font-size: 0.8em; color: #9d9d9d; }
  </style>
</head><body>
  <h2>M5Cardputer File Manager</h2>

  <h3>SD Card Status</h3>
  <div id="sd-status">
    <div><div class="label">Total</div><span id="total-space">...</span></div>
    <div><div class="label">Used</div><span id="used-space">...</span></div>
    <div><div class="label">Free</div><span id="free-space">...</span></div>
  </div>

  <h3>Files</h3>
  <table id="file-list">
    <thead><tr><th>Filename</th><th>Size (Bytes)</th><th>Action</th></tr></thead>
    <tbody></tbody>
  </table>

  <h3>Upload File</h3>
  <form id="upload-form" method="POST" action="/upload" enctype="multipart/form-data">
    <input type="file" name="upload">
    <input type="submit" value="Upload">
  </form>

  <script>
    function formatBytes(bytes, decimals = 2) {
        if (bytes === 0) return '0 Bytes';
        const k = 1024;
        const dm = decimals < 0 ? 0 : decimals;
        const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
    }

    function loadSdStatus() {
        fetch('/sdinfo')
            .then(response => response.json())
            .then(data => {
                const total = data.total || 0;
                const used = data.used || 0;
                const free = total - used;
                const percentUsed = total > 0 ? ((used / total) * 100).toFixed(1) : 0;
                const percentFree = total > 0 ? ((free / total) * 100).toFixed(1) : 0;

                document.getElementById('total-space').innerText = formatBytes(total);
                document.getElementById('used-space').innerText = `${formatBytes(used)} (${percentUsed}%)`;
                document.getElementById('free-space').innerText = `${formatBytes(free)} (${percentFree}%)`;
            });
    }

    function loadFiles() {
      fetch('/list')
        .then(response => response.json())
        .then(data => {
          const tbody = document.querySelector("#file-list tbody");
          tbody.innerHTML = '';
          data.forEach(file => {
            const row = `<tr>
              <td><a href="/${file.name}">${file.name}</a></td>
              <td>${file.size}</td>
              <td><button class="btn-delete" onclick="deleteFile('${file.name}')">Delete</button></td>
            </tr>`;
            tbody.innerHTML += row;
          });
        });
    }

    function deleteFile(filename) {
      if (confirm(`Are you sure you want to delete ${filename}?`)) {
        fetch(`/delete?file=${filename}`, { method: 'GET' })
          .then(response => {
              loadFiles();
              loadSdStatus();
          });
      }
    }
    
    document.querySelector('form').addEventListener('submit', function(e) {
        setTimeout(() => {
            loadFiles();
            loadSdStatus();
        }, 2000);
    });

    window.onload = () => {
        loadFiles();
        loadSdStatus();
    };
  </script>
</body></html>
)rawliteral";

void handleFileList() {
    File root = SD.open("/");
    String json = "[";

    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }
        if (!entry.isDirectory()) {
            if (json != "[") {
                json += ",";
            }
            // Use entry.name() to get the filename
            json += "{\"name\":\"" + String(entry.name()) + "\",\"size\":" + String(entry.size()) + "}";
        }
        entry.close();
    }
    json += "]";
    root.close();
    server.send(200, "application/json", json);
}

void handleFileDelete() {
    if (server.hasArg("file")) {
        String filePath = "/" + server.arg("file");
        if (SD.remove(filePath.c_str())) { // Use SD
            server.send(200, "text/plain", "File Deleted");
        } else {
            server.send(500, "text/plain", "Delete Failed");
        }
    } else {
        server.send(400, "text/plain", "Bad Request");
    }
}

void handleFileUpload(){
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
        String filename = "/" + upload.filename;
        if(SD.exists(filename)){ // Use SD
            SD.remove(filename); // Use SD
        }
        uploadFile = SD.open(filename.c_str(), FILE_WRITE); // Use SD
    } else if(upload.status == UPLOAD_FILE_WRITE){
        if(uploadFile)
            uploadFile.write(upload.buf, upload.currentSize);
    } else if(upload.status == UPLOAD_FILE_END){
        if(uploadFile)
            uploadFile.close();
        server.sendHeader("Location", "/");
        server.send(303);
    }
}

void handleNotFound() {
    String path = server.uri();
    if (SD.exists(path)) { // Use SD
        File file = SD.open(path.c_str(), FILE_READ); // Use File and SD
        if (file) {
            // ... (rest of function is the same)
        }
    }
    server.send(404, "text/plain", "Not Found");
}

void startWebServer() {
    #ifdef ENABLE_SD_CARD
    if (!isSdCardMounted) return;
    #endif

    server.on("/", HTTP_GET, [](){
        server.send_P(200, "text/html", index_html);
    });

    server.on("/sdinfo", HTTP_GET, [](){
        #ifdef ENABLE_SD_CARD
        if (!isSdCardMounted) {
            server.send(503, "application/json", "{\"error\":\"SD Card not mounted\"}");
            return;
        }
        
        // Use the updated SD library functions
        uint64_t totalBytes = SD.totalBytes();
        uint64_t usedBytes = SD.usedBytes();

        String json = "{";
        json += "\"total\":" + String(totalBytes) + ",";
        json += "\"used\":" + String(usedBytes);
        json += "}";
        
        server.send(200, "application/json", json);
        #else
        server.send(503, "application/json", "{\"error\":\"SD Card support disabled\"}");
        #endif
    });

    server.on("/list", HTTP_GET, handleFileList);
    server.on("/delete", HTTP_GET, handleFileDelete);
    
    server.on("/upload", HTTP_POST, [](){
        server.send(200);
    }, handleFileUpload);

    server.onNotFound(handleNotFound);

    server.begin();
}

void stopWebServer() {
    server.stop();
}

void handleWebServerClient() {
    server.handleClient();
}
#endif