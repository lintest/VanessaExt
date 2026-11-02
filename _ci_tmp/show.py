import json
import sys
import urllib.request

url = "https://api.github.com/repos/lintest/VanessaExt/actions/runs?per_page=3"
req = urllib.request.Request(url, headers={"User-Agent": "ci"})
data = json.load(urllib.request.urlopen(req, timeout=60))
for r in data.get("workflow_runs", []):
    print(r["run_number"], r["head_sha"][:7], r["status"], r["conclusion"], r["id"])
