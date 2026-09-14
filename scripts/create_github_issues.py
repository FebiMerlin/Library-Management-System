#!/usr/bin/env python3
"""Create GitHub Issues from docs/tracker_tasks.csv.

Usage:
    export GITHUB_TOKEN=ghp_...          # token with "repo" scope (PowerShell: $env:GITHUB_TOKEN="...")
    python scripts/create_github_issues.py OWNER/REPO [--assignee PM=login --assignee Dev=login ...] [--dry-run]

For every task the script creates an issue "T01: <title>" with labels
role:<role>, milestone by due date, and a body containing start/due dates,
estimate and dependencies. Existing issues with the same "Txx:" prefix are
skipped, so the script can be re-run safely.

Percent done and the kanban board are managed in GitHub Projects: add the
repository issues to a project, create a Number field "Progress" and a Board
view with columns Backlog / To do / In progress / Review / Done.

Only the standard library is used (no `requests`).
"""

import argparse
import csv
import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path

API = "https://api.github.com"


def request(method, url, token, data=None):
    body = json.dumps(data).encode() if data is not None else None
    req = urllib.request.Request(url, data=body, method=method)
    req.add_header("Authorization", f"Bearer {token}")
    req.add_header("Accept", "application/vnd.github+json")
    req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req) as resp:
            return json.loads(resp.read() or b"null")
    except urllib.error.HTTPError as e:
        sys.exit(f"{method} {url} -> {e.code}: {e.read().decode()}")


def existing_issue_titles(repo, token):
    titles = set()
    page = 1
    while True:
        items = request("GET", f"{API}/repos/{repo}/issues?state=all&per_page=100&page={page}", token)
        if not items:
            return titles
        titles.update(i["title"] for i in items if "pull_request" not in i)
        page += 1


def ensure_label(repo, token, name, color, existing):
    if name in existing:
        return
    request("POST", f"{API}/repos/{repo}/labels", token, {"name": name, "color": color})
    existing.add(name)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("repo", help="OWNER/REPO")
    parser.add_argument("--csv", default=Path(__file__).resolve().parent.parent / "docs" / "tracker_tasks.csv")
    parser.add_argument("--assignee", action="append", default=[], metavar="ROLE=LOGIN",
                        help="map a role (PM, DevOps, Dev) to a GitHub login; repeatable")
    parser.add_argument("--dry-run", action="store_true", help="print what would be created")
    args = parser.parse_args()

    token = os.environ.get("GITHUB_TOKEN")
    if not token and not args.dry_run:
        sys.exit("GITHUB_TOKEN is not set")

    logins = dict(a.split("=", 1) for a in args.assignee)

    with open(args.csv, encoding="utf-8", newline="") as f:
        tasks = list(csv.DictReader(f))

    role_colors = {"PM": "1d76db", "DevOps": "0e8a16", "Dev": "d93f0b"}
    existing_labels = set()
    existing_titles = set()
    if not args.dry_run:
        existing_labels = {l["name"] for l in request("GET", f"{API}/repos/{args.repo}/labels?per_page=100", token)}
        existing_titles = existing_issue_titles(args.repo, token)

    created = 0
    for t in tasks:
        title = f"{t['id']}: {t['title']}"
        if title in existing_titles:
            print(f"skip   {title} (exists)")
            continue
        roles = [r.strip() for r in t["role"].split(",")]
        labels = [f"role:{r}" for r in roles]
        assignees = [logins[r] for r in roles if r in logins]
        body = (
            f"**Роль:** {t['role']}\n"
            f"**Начало:** {t['start']}  **Срок:** {t['due']}  **Оценка:** {t['estimate_hours']} ч\n"
            f"**Зависит от:** {t['depends_on'] or '—'}\n\n"
            f"См. `docs/PLAN.md`. Процент выполнения — поле Progress в GitHub Projects."
        )
        if args.dry_run:
            print(f"create {title} labels={labels} assignees={assignees}")
            created += 1
            continue
        for r in roles:
            ensure_label(args.repo, token, f"role:{r}", role_colors.get(r, "ededed"), existing_labels)
        issue = request("POST", f"{API}/repos/{args.repo}/issues", token,
                        {"title": title, "body": body, "labels": labels, "assignees": assignees})
        print(f"created #{issue['number']} {title}")
        created += 1

    print(f"{created} issue(s) {'would be ' if args.dry_run else ''}created")


if __name__ == "__main__":
    main()
