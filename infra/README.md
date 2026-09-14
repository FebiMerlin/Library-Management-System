# Развёртывание инфраструктуры на ресурсах кафедры

Выполняет DevOps по задачам T10, T13, T14. Обоснование выбора — `docs/INFRASTRUCTURE.md`.

## Требования к хосту
Ubuntu Server 24.04 LTS, Docker ≥ 24 с плагином compose, 2 vCPU, 4 ГБ RAM, 20 ГБ диска, доступ по HTTP из сети университета (порты 3456, 3000, 2222, 8080 или reverse-proxy).

## Шаги

### 0. Подготовка
```bash
sudo apt-get install -y docker.io docker-compose-v2
git clone <url-форка> lms && cd lms/infra
cp .env.example .env && nano .env            # пароли и внешние адреса
```

### 1. Трекер (Vikunja)
```bash
docker compose --profile tracker up -d
# регистрация отключена — пользователей создаёт DevOps:
docker compose exec vikunja /app/vikunja/vikunja user create -u pm      -e pm@example.edu      -p PASSWORD
docker compose exec vikunja /app/vikunja/vikunja user create -u devops  -e devops@example.edu  -p PASSWORD
docker compose exec vikunja /app/vikunja/vikunja user create -u dev     -e dev@example.edu     -p PASSWORD
docker compose exec vikunja /app/vikunja/vikunja user create -u teacher -e teacher@example.edu -p PASSWORD
```
Далее в веб-интерфейсе (`http://host:3456`): проект «LMS» → «Поделиться» с участниками (teacher — только чтение) → вид Kanban → колонки Backlog / To do / In progress / Review / Done → завести задачи из `docs/tracker_tasks.csv` (поля: исполнитель, дата начала, срок, «Percent done»).

### 2. Git-зеркало (Gitea) и CI-раннер
```bash
docker compose --profile git --profile ci build lms-builder
docker compose --profile git --profile ci up -d
```
Первый вход в Gitea (`http://host:3000`) → создать администратора → Site administration → Actions → Runners → «Create new runner» → скопировать токен:
```bash
docker compose run --rm act-runner register --no-interactive \
    --instance http://gitea:3000 --token TOKEN --name lms-runner \
    --labels ubuntu-latest:docker://lms-builder:latest
docker compose --profile ci restart act-runner
```
Репозиторий: New migration → GitHub → URL форка → галочка «Mirror» (обновление каждые 8 ч). Gitea Actions выполняет `.github/workflows/ci.yml` без изменений; джобы Windows/macOS будут ждать раннера — это ожидаемо, полная матрица выполняется на GitHub.

### 3. IDE в браузере (по требованию Заказчика)
```bash
docker compose --profile ide up -d
```
`http://host:8080`, пароль из `.env`; проект смонтирован в `/home/coder/project`. Расширения из `.vscode/extensions.json` ставятся через палитру команд.

## Проверка
* `http://host:3456` — Vikunja: вход pm / devops / dev / teacher; канбан с 5 колонками; задачи с процентами.
* `http://host:3000` — Gitea: зеркало репозитория; вкладка Actions — зелёный прогон.
* `docker compose ps` — все сервисы в состоянии `running`.

## Резервное копирование
Раз в неделю (cron): `docker compose stop && tar czf lms-infra-$(date +%F).tgz /var/lib/docker/volumes/infra_* && docker compose start`; хранить 4 копии.

## Минимальный вариант без сервера кафедры
GitHub Issues + Projects (канбан, кастомное поле «Progress») и GitHub Actions — разворачивать ничего не нужно. Импорт задач: `python scripts/create_github_issues.py`.
