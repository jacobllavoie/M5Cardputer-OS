#ifdef ENABLE_WEB_SERVER
#include <M5CardputerOS_core.h>
#include <WebServer.h>
#include <ui.h>
#include "web_server.h"

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
let currentDir = '/';

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

function loadFiles(dir = '/') {
    currentDir = dir;
    fetch(`/list?dir=${encodeURIComponent(dir)}`)
        .then(response => response.json())
        .then(data => {
            const tbody = document.querySelector("#file-list tbody");
            tbody.innerHTML = '';
            if (dir !== '/') {
                tbody.innerHTML += `<tr>
                    <td colspan="3"><button onclick="loadFiles('${dir.substring(0, dir.lastIndexOf('/')) || '/'}')">⬅️ Up</button></td>
                </tr>`;
            }
            data.forEach(file => {
                if (file.isDir === "true") {
                    tbody.innerHTML += `<tr>
                        <td><a href="#" onclick="event.preventDefault(); loadFiles('${dir}/${file.name}')">${file.name}/</a></td>
                        <td>DIR</td>
                        <td>
                            <button onclick="deleteFile('${dir}/${file.name}')">Delete</button>
                            <button onclick="moveFilePrompt('${dir}/${file.name}')">Move</button>
                            <button onclick="copyFilePrompt('${dir}/${file.name}')">Copy</button>
                        </td>
                    </tr>`;
                } else {
                    tbody.innerHTML += `<tr>
                        <td><a href="/${file.name}">${file.name}</a></td>
                        <td>${file.size}</td>
                        <td>
                            <button onclick="deleteFile('${dir}/${file.name}')">Delete</button>
                            <button onclick="moveFilePrompt('${dir}/${file.name}')">Move</button>
                            <button onclick="copyFilePrompt('${dir}/${file.name}')">Copy</button>
                        </td>
                    </tr>`;
                }
            });
        });
}

function deleteFile(path) {
    if (confirm(`Are you sure you want to delete ${path}?`)) {
        fetch(`/delete?file=${encodeURIComponent(path)}`, { method: 'GET' })
            .then(() => {
                loadFiles(currentDir);
                loadSdStatus();
            });
    }
}

function moveFilePrompt(path) {
    const to = prompt("Move to (full path):", path);
    if (to && to !== path) {
        fetch(`/move?from=${encodeURIComponent(path)}&to=${encodeURIComponent(to)}`, { method: 'GET' })
            .then(() => loadFiles(currentDir));
    }
}

function copyFilePrompt(path) {
    const to = prompt("Copy to (full path):", path + '_copy');
    if (to && to !== path) {
        fetch(`/copy?from=${encodeURIComponent(path)}&to=${encodeURIComponent(to)}`, { method: 'GET' })
            .then(() => loadFiles(currentDir));
    }
}

function createFolderPrompt() {
    const folderName = prompt("New folder name:");
    if (folderName) {
        fetch(`/mkdir?dir=${encodeURIComponent(currentDir + '/' + folderName)}`, { method: 'GET' })
            .then(() => loadFiles(currentDir));
    }
}

document.querySelector('form').addEventListener('submit', function(e) {
    e.preventDefault();
    const formData = new FormData(this);
    fetch(`/upload?dir=${encodeURIComponent(currentDir)}`, {
        method: 'POST',
        body: formData
    }).then(() => {
        setTimeout(() => {
            loadFiles(currentDir);
            loadSdStatus();
        }, 2000);
    });
});

window.onload = () => {
    // Use the current URL path as the initial directory
    let initialDir = decodeURIComponent(window.location.pathname);
    if (!initialDir || initialDir === '') initialDir = '/';
    loadFiles(initialDir);
    loadSdStatus();
    // Add create folder button
    const h3 = document.querySelector('h3');
    const btn = document.createElement('button');
    btn.textContent = 'Create Folder';
    btn.onclick = createFolderPrompt;
    h3.parentNode.insertBefore(btn, h3.nextSibling);
};
</script>
</body></html>
)rawliteral";

void handleFileList() {
    String dir = "/";
    if (server.hasArg("dir")) {
        dir = server.arg("dir");
        if (!dir.startsWith("/")) dir = "/" + dir;
    }
    File root = SD.open(dir);
    String json = "[";
    while (true) {
        File entry = root.openNextFile();
        if (!entry) {
            break;
        }
        if (json != "[") {
            json += ",";
        }
        json += "{\"name\":\"" + String(entry.name()) + "\",";
        json += "\"size\":" + String(entry.size()) + ",";
        json += "\"isDir\":" + String(entry.isDirectory() ? "true" : "false") + "}";
        entry.close();
    }
    json += "]";
    root.close();
    server.send(200, "application/json", json);
}

void handleMkdir() {
    String dir = server.arg("dir");
    if (!dir.startsWith("/")) dir = "/" + dir;
    if (SD.mkdir(dir)) {
        server.send(200, "text/plain", "Folder created");
    } else {
        server.send(500, "text/plain", "Failed to create folder");
    }
}

void handleMove() {
    String from = server.arg("from");
    String to = server.arg("to");
    if (!from.startsWith("/")) from = "/" + from;
    if (!to.startsWith("/")) to = "/" + to;
    if (SD.rename(from.c_str(), to.c_str())) {
        server.send(200, "text/plain", "Moved");
    } else {
        server.send(500, "text/plain", "Move failed");
    }
}

void handleCopy() {
    String from = server.arg("from");
    String to = server.arg("to");
    if (!from.startsWith("/")) from = "/" + from;
    if (!to.startsWith("/")) to = "/" + to;
    File src = SD.open(from.c_str(), FILE_READ);
    File dst = SD.open(to.c_str(), FILE_WRITE);
    if (!src || !dst) {
        server.send(500, "text/plain", "Copy failed");
        if (src) src.close();
        if (dst) dst.close();
        return;
    }
    uint8_t buf[512];
    size_t n;
    while ((n = src.read(buf, sizeof(buf))) > 0) {
        dst.write(buf, n);
    }
    src.close();
    dst.close();
    server.send(200, "text/plain", "Copied");
}

void handleFileDelete() {
    debugMessage("DEBUG:", "handleFileDelete() called");
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
    debugMessage("DEBUG:", "handleFileUpload() called");
    HTTPUpload& upload = server.upload();
    if(upload.status == UPLOAD_FILE_START){
        String filename = "/" + upload.filename;
    debugMessage("DEBUG:", "Upload START: " + filename);

        if(SD.exists(filename)){
            SD.remove(filename);
            debugMessage("DEBUG:", "Existing file removed.");
        }
        uploadFile = SD.open(filename.c_str(), FILE_WRITE);
        if (!uploadFile) {
            debugMessage("DEBUG:", "Failed to create file for writing!");
            return;
        }
    debugMessage("DEBUG:", "File created for writing.");

    } else if(upload.status == UPLOAD_FILE_WRITE){
        if(uploadFile){
            size_t bytesWritten = uploadFile.write(upload.buf, upload.currentSize);
            debugMessage("DEBUG:", "Writing chunk: " + String(bytesWritten) + " bytes");
            if (bytesWritten != upload.currentSize) {
                debugMessage("DEBUG:", "File write failed!");
            }
        }
    } else if(upload.status == UPLOAD_FILE_END){
        if(uploadFile){
            uploadFile.close();
            debugMessage("DEBUG:", "Upload END. Total size: " + String(upload.totalSize));
        }
        server.sendHeader("Location", "/");
        server.send(303); // Redirect back to the main page
    }
}

void handleNotFound() {
    debugMessage("DEBUG:", "handleNotFound() called");
    debugMessage("DEBUG:", "Requested URI: " + server.uri());
    String path = server.uri();
    #ifdef ENABLE_SD_CARD
    if (SD.exists(path)) {
        File f = SD.open(path);
        if (f && f.isDirectory()) {
            f.close();
            server.send_P(200, "text/html", index_html);
            return;
        }
        if (f) f.close();
    }
    #endif
    server.send(404, "text/plain", "Not Found: " + path);
}

void handleFolderRequest() {
    String dir = server.uri();
    if (!SD.exists(dir) || !SD.open(dir).isDirectory()) {
        server.send(404, "text/plain", "Not Found: " + dir);
        return;
    }
    server.send_P(200, "text/html", index_html);
}

void handleFileServe() {
    String path = server.uri();
    debugMessage("DEBUG:", "handleFileServe() called for path: " + path);
    debugMessage("DEBUG:", "SD.exists(path) = " + String(SD.exists(path) ? "true" : "false"));
    if (SD.exists(path)) {
        File file = SD.open(path.c_str(), FILE_READ);
        if (file && !file.isDirectory()) {
            String contentType = "application/octet-stream";
            if (path.endsWith(".jpg")) contentType = "image/jpeg";
            else if (path.endsWith(".png")) contentType = "image/png";
            else if (path.endsWith(".gif")) contentType = "image/gif";
            else if (path.endsWith(".txt")) contentType = "text/plain";
            else if (path.endsWith(".html")) contentType = "text/html";
            server.streamFile(file, contentType);
            file.close();
            return;
        }
        if (file) file.close();
    }
    server.send(404, "text/plain", "File Not Found: " + path);
}

void startWebServer() {
    debugMessage("DEBUG:", "startWebServer() called");
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
    server.on("/mkdir", HTTP_GET, handleMkdir);
    server.on("/move", HTTP_GET, handleMove);
    server.on("/copy", HTTP_GET, handleCopy);
    server.on("/delete", HTTP_GET, handleFileDelete);
    server.on("/upload", HTTP_POST, [](){
        server.send(200);
    }, handleFileUpload);
    // Catch-all for folder paths
    server.on("/apps", HTTP_GET, handleFolderRequest);
    server.on("/test", HTTP_GET, handleFolderRequest);
    // Optionally, add more or use a wildcard if supported
    server.onNotFound(handleFileServe);
    server.begin();
}

void stopWebServer() {
    debugMessage("DEBUG:", "stopWebServer() called");
    server.stop();
}

void handleWebServerClient() {
    debugMessage("DEBUG:", "handleWebServerClient() called");
    server.handleClient();
}
#endif