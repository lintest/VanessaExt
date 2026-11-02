import os, sys, urllib.request, urllib.error

token = os.environ.get("GH_TOKEN", "")
job = sys.argv[1]
out = sys.argv[2]
url = f"https://api.github.com/repos/lintest/VanessaExt/actions/jobs/{job}/logs"
req = urllib.request.Request(url, headers={
    "Authorization": f"Bearer {token}",
    "User-Agent": "ci",
    "Accept": "application/vnd.github+json",
})
try:
    with urllib.request.urlopen(req, timeout=120) as r:
        data = r.read()
    with open(out, "wb") as f:
        f.write(data)
    print("ok", len(data))
except urllib.error.HTTPError as e:
    print("HTTP", e.code, e.read()[:300])
except Exception as e:
    print("ERR", type(e).__name__, e)
