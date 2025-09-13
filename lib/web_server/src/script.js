document.addEventListener('DOMContentLoaded', () => {
    const ipAddressSpan = document.getElementById('ipAddress');
    const cpuUtilizationSpan = document.getElementById('cpuUtilization');
    const batteryPercentageSpan = document.getElementById('batteryPercentage');
    const sdCardUsageSpan = document.getElementById('sdCardUsage');
    const fileListBody = document.getElementById('fileList');
    const uploadForm = document.getElementById('upload-form');
    const uploadFile = document.getElementById('uploadFile');

    const formatBytes = (bytes) => {
        if (bytes === 0) return '0 Bytes';
        const k = 1024;
        const sizes = ['Bytes', 'KB', 'MB', 'GB', 'TB'];
        const i = Math.floor(Math.log(bytes) / Math.log(k));
        return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
    };

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

    async function updateFileList() {
        try {
            const response = await fetch('/files');
            const files = await response.json();
            fileListBody.innerHTML = '';
            files.forEach(file => {
                const row = fileListBody.insertRow();
                const displayName = file.name.startsWith('/') ? file.name.substring(1) : file.name;
                row.insertCell().innerHTML = `<a href="/${displayName}" target="_blank">${displayName}</a>`;
                row.insertCell().textContent = formatBytes(file.size);
                const actionCell = row.insertCell();
                actionCell.innerHTML = `<button onclick="deleteFile('${file.name}')">Delete</button>`;
            });
        } catch (error) { console.error('Failed to fetch file list:', error); }
    }
    
    window.deleteFile = async (fileName) => {
        if (confirm(`Are you sure you want to delete ${fileName}?`)) {
            try {
                const response = await fetch(`/delete?file=${encodeURIComponent(fileName)}`, { method: 'POST' });
                if (response.ok) {
                    updateFileList();
                } else { alert('Failed to delete file.'); }
            } catch (error) { console.error('Error deleting file:', error); }
        }
    }

    uploadForm.addEventListener('submit', function(e) {
        e.preventDefault();
        const file = uploadFile.files[0];
        if (!file) {
            alert('Please select a file to upload.');
            return;
        }
        const formData = new FormData();
        formData.append(file.name, file);

        fetch('/upload', { method: 'POST', body: formData })
        .then(response => {
            if (response.ok) {
                 // The server will redirect, but we can also manually refresh
                 setTimeout(() => window.location.reload(), 1000);
            } else {
                alert('File upload failed.');
            }
        }).catch(error => console.error('Error uploading file:', error));
    });

    updateStats();
    updateFileList();
    setInterval(updateStats, 5000);
});

