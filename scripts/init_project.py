#!/usr/bin/python3

import os.path
import requests

temp_dir = 'temp'

res_urls = [
    'https://filesamples.com/samples/audio/mp2/sample1.mp2',
    'https://sample-videos.com/video123/mp4/720/big_buck_bunny_720p_1mb.mp4',
]

if __name__ == '__main__':
    if not os.path.exists(temp_dir):
        os.makedirs(temp_dir)
    for url in res_urls:
        filename = url.split('/')[-1]
        url_path = os.path.join(os.getcwd(), os.path.join(temp_dir, filename))
        if not os.path.exists(url_path):
            r = requests.get(url, allow_redirects=True)
            open(url_path, 'wb').write(r.content)
