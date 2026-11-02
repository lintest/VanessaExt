import json
import time
import urllib.request

sha = "974efaf"
deadline = time.time() + 60 * 45

while time.time() < deadline:
    url = "https://api.github.com/repos/lintest/VanessaExt/actions/runs?per_page=3"
    req = urllib.request.Request(url, headers={"User-Agent": "ci"})
    data = json.load(urllib.request.urlopen(req, timeout=60))
    run = None
    for r in data.get("workflow_runs", []):
        if r["head_sha"].startswith(sha):
            run = r
            break
    if run is None:
        print("прогон для", sha, "не найден")
        break
    print(time.strftime("%H:%M:%S"), "run", run["run_number"], run["status"], run["conclusion"], flush=True)
    if run["status"] == "completed":
        print("ИТОГ:", run["conclusion"], run["html_url"])
        break
    time.sleep(60)
