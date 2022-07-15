#!/usr/bin/python3
import os
import requests

DOWNLOAD_FILES = {
    'build/build_config.h': 'https://raw.githubusercontent.com/chromium/chromium/main/build/build_config.h',
    'build/buildflag.h': 'https://raw.githubusercontent.com/chromium/chromium/main/build/buildflag.h',
}

if __name__ == '__main__':
    for file in DOWNLOAD_FILES.keys():
        file_path = os.path.join(os.getcwd(), file)
        if not os.path.exists(file_path):
            r = requests.get(DOWNLOAD_FILES[file], allow_redirects=True)
            open(file_path, 'wb').write(r.content)
