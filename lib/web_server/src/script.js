document.addEventListener('DOMContentLoaded', () => {
    // --- DOM Element References ---
    const ipAddressSpan = document.getElementById('ipAddress');
    const cpuUtilizationSpan = document.getElementById('cpuUtilization');
    const batteryPercentageSpan = document.getElementById('batteryPercentage');
    const sdCardUsageSpan = document.getElementById('sdCardUsage');
    const fileListBody = document.getElementById('fileList');
    const uploadForm = document.getElementById('upload-form');

    // --- State ---
    let currentDir = '/';

    // --- Helper Functions ---
    const formatBytes = (bytes) => {
        if (bytes === 0) return '0 Bytes';
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    };

    // --- API Calls ---
    async function updateStats() {
        try {
            const response = await fetch('/stats');
            const data = await response.json();
            ipAddressSpan.innerHTML = `${data.ip}<span class="cursor">_</span>`;
            cpuUtilizationSpan.innerHTML = `${data.cpuUtilization}%<span class="cursor">_</span>`;
            batteryPercentageSpan.innerHTML = `${data.batteryPercentage}%<span class="cursor">_</span>`;
            const sdUsed = data.sdUsedBytes;
            const sdTotal = data.sdTotalBytes;
            const sdPercentage = sdTotal > 0 ? ((sdUsed / sdTotal) * 100).toFixed(1) : 0;
            sdCardUsageSpan.innerHTML = `${sdPercentage}% USED (${formatBytes(sdUsed)}/${formatBytes(sdTotal)})<span class="cursor">_</span>`;
        } catch (error) { console.error('Failed to fetch stats:', error); }
    }

    async function updateFileList(dir = '/') {
        currentDir = dir;
        try {
            const response = await fetch(`/files?dir=${encodeURIComponent(dir)}`);
            const files = await response.json();
            fileListBody.innerHTML = ''; // Clear current list

            // Add "Up" button if not in root
            if (dir !== '/') {
                const parentDir = dir.substring(0, dir.lastIndexOf('/')) || '/';
                const upRow = fileListBody.insertRow();
                upRow.insertCell().innerHTML = `<a href="#" onclick="updateFileList('${parentDir}')">.. [Up]</a>`;
                upRow.insertCell();
                upRow.insertCell();
            }
            
            // Sort files: directories first, then alphabetically
            files.sort((a, b) => {
                if (a.isDir !== b.isDir) {
                    return a.isDir ? -1 : 1;
                }
                return a.name.localeCompare(b.name);
            });

            files.forEach(file => {
                const row = fileListBody.insertRow();
                const fullPath = (dir === '/' ? '' : dir) + '/' + file.name.split('/').pop();

                if (file.isDir) {
                    row.insertCell().innerHTML = `<a href="#" onclick="updateFileList('${fullPath}')">${file.name.split('/').pop()}/</a>`;
                    row.insertCell().textContent = 'DIR';
                } else {
                    row.insertCell().innerHTML = `<a href="${fullPath}" target="_blank">${file.name.split('/').pop()}</a>`;
                    row.insertCell().textContent = formatBytes(file.size);
                }
                
                const actionCell = row.insertCell();
                actionCell.innerHTML = `
                    <button onclick="moveFile('${fullPath}')">Move/Rename</button>
                    <button onclick="deleteFile('${fullPath}')">Delete</button>
                `;
            });
        } catch (error) { console.error('Failed to fetch file list:', error); }
    }

    // --- Global Functions for Buttons ---
    window.deleteFile = async (path) => {
        if (confirm(`Are you sure you want to delete ${path}?`)) {
            try {
                await fetch(`/delete?path=${encodeURIComponent(path)}`, { method: 'POST' });
                updateFileList(currentDir);
            } catch (error) { console.error('Error deleting file:', error); }
        }
    }

    window.moveFile = async (fromPath) => {
        const toPath = prompt('Enter new full path (e.g., /new_folder/new_name.txt):', fromPath);
        if (toPath && toPath !== fromPath) {
            try {
                await fetch(`/move?from=${encodeURIComponent(fromPath)}&to=${encodeURIComponent(toPath)}`, { method: 'POST' });
                updateFileList(currentDir);
            } catch (error) { console.error('Error moving file:', error); }
        }
    }
    
    // Make updateFileList globally accessible for the "Up" button
    window.updateFileList = updateFileList;

    // --- Event Listeners ---
    uploadForm.addEventListener('submit', function(e) {
        e.preventDefault();
        const formData = new FormData(this);
        fetch(`/upload?dir=${encodeURIComponent(currentDir)}`, { method: 'POST', body: formData })
        .then(response => {
            if (response.ok) {
                alert('File uploaded successfully!');
                updateFileList(currentDir);
            } else {
                alert('File upload failed.');
            }
        }).catch(error => console.error('Error uploading file:', error));
    });

    // --- Initial Load ---
    updateStats();
    updateFileList('/');
    setInterval(updateStats, 5000);
});

