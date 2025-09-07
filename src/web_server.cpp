#include "globals.h"
#include <WebServer.h>
#include "ui.h"

WebServer server(80);
FsFile uploadFile;

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
    FsFile root = sd.open("/");
    String json = "[";
    char fileName[256];

    while (true) {
        FsFile entry = root.openNextFile();
        if (!entry) {
            break;
        }
        if (!entry.isDirectory()) {
            if (json != "[") {
                json += ",";
            }
            entry.getName(fileName, sizeof(fileName));
            json += "{\"name\":\"" + String(fileName) + "\",\"size\":" + String(entry.size()) + "}";
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
        if (sd.remove(filePath.c_str())) {
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
        if(sd.exists(filename)){
            sd.remove(filename);
        }
        uploadFile = sd.open(filename.c_str(), O_WRONLY | O_CREAT);
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
    if (sd.exists(path)) {
        FsFile file = sd.open(path.c_str(), O_RDONLY);
        if (file) {
            server.setContentLength(file.size());
            server.sendHeader("Content-Disposition", "attachment");
            server.send(200, "application/octet-stream", "");
            uint8_t buffer[1460];
            while(file.available()) {
                size_t bytesRead = file.read(buffer, sizeof(buffer));
                server.sendContent((const char*)buffer, bytesRead);
            }
            file.close();
            return;
        }
    }
    server.send(404, "text/plain", "Not Found");
}

void startWebServer() {
    if (!isSdCardMounted) return;

    server.on("/", HTTP_GET, [](){
        server.send_P(200, "text/html", index_html);
    });

    server.on("/sdinfo", HTTP_GET, [](){
        if (!isSdCardMounted) {
            server.send(503, "application/json", "{\"error\":\"SD Card not mounted\"}");
            return;
        }
        
        FsVolume* vol = sd.vol();
        uint32_t clusterCount = vol->clusterCount();
        uint32_t freeClusterCount = vol->freeClusterCount();
        uint32_t clusterSize = vol->bytesPerCluster();

        uint64_t totalBytes = (uint64_t)clusterCount * clusterSize;
        uint64_t freeBytes = (uint64_t)freeClusterCount * clusterSize;
        uint64_t usedBytes = totalBytes - freeBytes;

        String json = "{";
        json += "\"total\":" + String(totalBytes) + ",";
        json += "\"used\":" + String(usedBytes);
        json += "}";
        
        server.send(200, "application/json", json);
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