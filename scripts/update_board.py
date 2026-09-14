#!/usr/bin/env python3
"""Set Status and Progress of every task on the GitHub Projects board.

Usage:
    export GITHUB_TOKEN=ghp_...   # classic token with scopes: repo, project
    python scripts/update_board.py OWNER PROJECT_NUMBER [--rules rules.csv] [--dry-run]

Default rules (when --rules is not given): every "Txx:" issue -> Done / 100,
except the ones listed in DEFAULT_OPEN below. Issues set to Done are also
closed, so the repository issue list matches the board.

A rules file is CSV with columns: id,status,progress  (e.g. T07,In Progress,50).

Requires a *classic* personal access token: fine-grained tokens cannot
access Projects (v2). Only the standard library is used.
"""

import argparse
import csv
import json
import os
import sys
import time
import urllib.error
import urllib.request

API = "https://api.github.com"

# id -> (status, progress) for tasks that are not finished at hand-over time.
DEFAULT_OPEN = {
    "T07": ("In Progress", 50),  # согласование инфраструктуры с Заказчиком
    "T08": ("In Progress", 50),  # согласование ТЗ
    "T38": ("In Progress", 50),  # контроль качества на чистой машине
    "T40": ("Todo", 0),          # сдача проекта
}


def call(url, token, method="GET", data=None):
    body = json.dumps(data).encode() if data is not None else None
    # GitHub throttles bursts of mutations (secondary rate limit, HTTP 403/429)
    # and occasionally returns 5xx; retry with growing pauses before giving up.
    for attempt in range(6):
        req = urllib.request.Request(url, data=body, method=method)
        req.add_header("Authorization", f"Bearer {token}")
        req.add_header("Accept", "application/vnd.github+json")
        req.add_header("Content-Type", "application/json")
        try:
            with urllib.request.urlopen(req) as resp:
                return json.loads(resp.read() or b"null")
        except urllib.error.HTTPError as e:
            text = e.read().decode()
            if e.code in (403, 429, 500, 502, 503, 504) and attempt < 5:
                wait = int(e.headers.get("Retry-After", 0) or 0) or 10 * (attempt + 1)
                print(f"  {e.code} from GitHub, retrying in {wait}s ...")
                time.sleep(wait)
                continue
            sys.exit(f"{method} {url} -> {e.code}: {text}")
        except urllib.error.URLError as e:
            if attempt < 5:
                print(f"  network error ({e.reason}), retrying in 10s ...")
                time.sleep(10)
                continue
            raise


def graphql(query, token, **variables):
    result = call(f"{API}/graphql", token, "POST", {"query": query, "variables": variables})
    if result.get("errors"):
        sys.exit("GraphQL error: " + json.dumps(result["errors"], ensure_ascii=False, indent=2))
    return result["data"]


PROJECT_QUERY = """
query($owner: String!, $number: Int!, $after: String) {
  user(login: $owner) {
    projectV2(number: $number) {
      id
      fields(first: 50) {
        nodes {
          ... on ProjectV2SingleSelectField { id name options { id name } }
          ... on ProjectV2Field { id name dataType }
        }
      }
      items(first: 100, after: $after) {
        pageInfo { hasNextPage endCursor }
        nodes {
          id
          content { ... on Issue { number title repository { nameWithOwner } } }
        }
      }
    }
  }
}
"""

UPDATE_MUTATION = """
mutation($project: ID!, $item: ID!, $field: ID!, $value: ProjectV2FieldValue!) {
  updateProjectV2ItemFieldValue(input: {projectId: $project, itemId: $item, fieldId: $field, value: $value}) {
    projectV2Item { id }
  }
}
"""


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("owner")
    parser.add_argument("number", type=int)
    parser.add_argument("--rules", help="CSV file: id,status,progress")
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN")
    if not token:
        sys.exit("GITHUB_TOKEN is not set")

    rules = dict(DEFAULT_OPEN)
    if args.rules:
        with open(args.rules, encoding="utf-8", newline="") as f:
            for row in csv.DictReader(f):
                rules[row["id"].strip()] = (row["status"].strip(), int(row["progress"]))

    # ---- read the project: fields, options, items (paged) ----
    items, project, fields = [], None, None
    after = None
    while True:
        data = graphql(PROJECT_QUERY, token, owner=args.owner, number=args.number, after=after)
        p = data["user"]["projectV2"]
        if p is None:
            sys.exit("Project not found (check owner, number and token scopes: repo, project)")
        project, fields = p["id"], p["fields"]["nodes"]
        items.extend(p["items"]["nodes"])
        if not p["items"]["pageInfo"]["hasNextPage"]:
            break
        after = p["items"]["pageInfo"]["endCursor"]

    status = next((f for f in fields if f.get("name") == "Status" and "options" in f), None)
    progress = next((f for f in fields if f.get("name") == "Progress"), None)
    if not status:
        sys.exit("Status field not found")
    if not progress:
        sys.exit("Progress field not found - create a Number field named 'Progress' in the project")
    options = {o["name"]: o["id"] for o in status["options"]}

    done_count = 0
    for it in items:
        c = it.get("content") or {}
        title = c.get("title", "")
        if not title[:3].startswith("T") or ":" not in title:
            continue
        tid = title.split(":")[0].strip()
        st, pct = rules.get(tid, ("Done", 100))
        if st not in options:
            sys.exit(f"Status option '{st}' does not exist; have: {list(options)}")
        print(f"{tid:4} -> {st:12} {pct:3}%  (#{c.get('number')})")
        if args.dry_run:
            continue
        graphql(UPDATE_MUTATION, token, project=project, item=it["id"], field=status["id"],
                value={"singleSelectOptionId": options[st]})
        graphql(UPDATE_MUTATION, token, project=project, item=it["id"], field=progress["id"],
                value={"number": pct})
        if st == "Done":
            call(f"{API}/repos/{c['repository']['nameWithOwner']}/issues/{c['number']}", token, "PATCH",
                 {"state": "closed", "state_reason": "completed"})
            done_count += 1
        time.sleep(1.5)  # stay under the secondary rate limit

    print(f"updated {len([1 for i in items if (i.get('content') or {}).get('title','').startswith('T')])} items, "
          f"closed {done_count} issues{' (dry run)' if args.dry_run else ''}")


if __name__ == "__main__":
    main()
