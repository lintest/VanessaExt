import json
import time
import urllib.request

deadline = time.time() + 60 * 40
last = None

def get(url):
    req = urllib.request.Request(url, headers={"User-Agent": "ci"})
    return json.load(urllib.request.urlopen(req, timeout=60))

while time.time() < deadline:
    data = get("https://api.github.com/repos/lintest/VanessaExt/actions/runs?per_page=1")
    r = data["workflow_runs"][0]
    line = (time.strftime("%H:%M:%S"), r["run_number"], r["head_sha"][:7],
            r["status"], r["conclusion"])
    print(*line, flush=True)
    if r["status"] == "completed":
        print("ИТОГ:", r["conclusion"], r["html_url"], "id", r["id"])
        break
    time.sleep(60)
