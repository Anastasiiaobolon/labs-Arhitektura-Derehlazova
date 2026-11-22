# Lab 7. Pipes, Redirection, Signals та Mini-Shell

**Середовище:** GitHub Codespaces, Ubuntu, GCC, `strace`.
Executable-файли та згенеровані файли (`redirected.txt`, `ls-output.txt`, `output.txt`) в репозиторій не додаються.

## Файли

`pipe_basic.c`, `pipe_processes.c`, `pipe_no_close.c`, `dup2_output.c`, `redirect_exec.c`, `pipeline.c`, `signal_int.c`, `signal_parent_child.c`, `mini_shell.c`.

Усі програми компілюються так: `gcc -Wall -Wextra file.c -o file`.

# Частина I. Pipes

## Завдання 1. Pipe в одному процесі (`pipe_basic.c`)

```bash
gcc -Wall -Wextra pipe_basic.c -o pipe_basic && ./pipe_basic
```
```text
Read end:  3
Write end: 4
Received: Hello through pipe!
```
`pipe(pipefd)` створює канал у ядрі та два дескриптори: `pipefd[0]` читання, `pipefd[1]` запис. Номери 3 та 4, бо 0, 1, 2 уже зайняті. Що записано в write end, можна прочитати з read end.

## Завдання 2. Pipe між parent і child (`pipe_processes.c`)

```bash
./pipe_processes
```
```text
Child received: Message from parent
```
Після `fork()` child успадковує обидва дескриптори. Parent закриває read end і пише, child закриває write end і читає. **Чому закриваються невикористані кінці?** Кожен процес залишає лише той кінець, який йому потрібен: це робить напрямок даних однозначним і потрібно для коректного EOF (див. далі).

## Завдання 3. Що буде без `close()` (`pipe_no_close.c`)

```bash
gcc -Wall -Wextra pipe_no_close.c -o pipe_no_close
timeout 5 ./pipe_no_close ; echo "exit code: $?"      # зависає, timeout завершує з кодом 124

gcc -Wall -Wextra -DCLOSE_UNUSED pipe_no_close.c -o pipe_ok
./pipe_ok
```
```text
Child received: Message from parent
exit code: 124
Child received: Message from parent
Child: EOF reached (read returned 0)
```
Очікується: у версії **без** закриття child прочитає повідомлення, а потім зависне на наступному `read()` (рядка `EOF reached` немає). Версія з `CLOSE_UNUSED` виводить `Child: EOF reached (read returned 0)`.

**Чому pipe empty ≠ EOF?** Якщо pipe порожній, але ще **існує хоча б один відкритий write end** (навіть у самого child, який читає), `read()` блокується і чекає нових даних. EOF (`read() == 0`) настає лише коли pipe порожній **і всі write ends закриті**.

## Завдання 4. `strace` для pipe

```bash
strace -f -e trace=pipe,pipe2,clone,fork,vfork,read,write,close,wait4 ./pipe_processes
```
```text
pipe2([3, 4], 0) = 0
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x703f94ce7a10) = 29911
[pid 29910] close(3) = 0
[pid 29911] close(4) = 0
[pid 29910] write(4, "Message from parent\n", 20) = 20
[pid 29911] read(3, "Message from parent\n", 127) = 20
[pid 29910] close(4) = 0
[pid 29910] wait4(29911, NULL, 0, NULL) = 29911
[pid 29911] close(3) = 0
```
Послідовність: `pipe2` (створення), `clone` (fork), `close` (зайві кінці), `write` (parent), `read` (child), `close`, `wait4` (очікування child).

# Частина II. Redirection

## Завдання 5. `dup2()` (`dup2_output.c`)

```bash
./dup2_output ; cat redirected.txt
```
```text
This goes to the file
```
На екран нічого не виводиться. `dup2(fd, STDOUT_FILENO)` робить так, що дескриптор 1 тепер вказує на той самий файл, що й `fd`. `printf` пише в stdout (дескриптор 1), тому текст потрапляє у файл. **Змінився ресурс, на який посилається stdout**, а не сам код `printf`. Оригінальний `fd` потім закривається, бо він більше не потрібен.

## Завдання 6. `exec()` + redirection (`redirect_exec.c`)

```bash
./redirect_exec ; cat ls-output.txt
```
```text
Child finished, output saved to ls-output.txt
total 244
-rw-rw-rw- 1 codespace codespace 14511 Nov 22 05:44 README.md
-rwxrwxrwx 1 codespace codespace 16216 Nov 22 05:39 dup2_output
-rw-rw-rw- 1 codespace codespace   497 Nov 21 22:46 dup2_output.c
-rw-r--r-- 1 codespace codespace     0 Nov 22 05:44 ls-output.txt
-rwxrwxrwx 1 codespace codespace 21712 Nov 22 05:39 mini_shell
-rw-rw-rw- 1 codespace codespace  8257 Nov 21 22:46 mini_shell.c
-rwxrwxrwx 1 codespace codespace 16224 Nov 22 05:39 pipe_basic
-rw-rw-rw- 1 codespace codespace   839 Nov 21 22:46 pipe_basic.c
-rwxrwxrwx 1 codespace codespace 16440 Nov 22 05:39 pipe_no_close
-rw-rw-rw- 1 codespace codespace  1655 Nov 21 22:46 pipe_no_close.c
-rwxrwxrwx 1 codespace codespace 16440 Nov 22 05:40 pipe_ok
-rwxrwxrwx 1 codespace codespace 16360 Nov 22 05:39 pipe_processes
-rw-rw-rw- 1 codespace codespace  1099 Nov 21 22:46 pipe_processes.c
-rwxrwxrwx 1 codespace codespace 16312 Nov 22 05:39 pipeline
-rw-rw-rw- 1 codespace codespace  1699 Nov 21 22:46 pipeline.c
-rwxrwxrwx 1 codespace codespace 16360 Nov 22 05:39 redirect_exec
-rw-rw-rw- 1 codespace codespace   983 Nov 21 22:46 redirect_exec.c
-rw-r--r-- 1 codespace codespace    22 Nov 22 05:44 redirected.txt
-rwxrwxrwx 1 codespace codespace 16192 Nov 22 05:39 signal_int
-rw-rw-rw- 1 codespace codespace   565 Nov 21 22:46 signal_int.c
-rwxrwxrwx 1 codespace codespace 16408 Nov 22 05:39 signal_parent_child
-rw-rw-rw- 1 codespace codespace  1690 Nov 21 22:46 signal_parent_child.c
```
Child відкриває `ls-output.txt`, `dup2` перенаправляє stdout, потім `execvp("ls", ...)`. Перенаправлення **зберігається після `exec`**, бо дескриптори переживають заміну програми, тому `ls` пише у файл, не знаючи про це. Parent викликає `waitpid()`.

## Завдання 7. Pipeline `ls -l | wc -l` (`pipeline.c`)

```bash
./pipeline
ls -l | wc -l      # для порівняння
```
```text
./pipeline      → 23
ls -l | wc -l   → 23
```
Схема:
```text
Parent ── pipe()
  ├─ Child 1: stdout → pipe  ── exec(ls -l)
  └─ Child 2: stdin ← pipe   ── exec(wc -l)
Parent: close(обидва кінці), waitpid(child1), waitpid(child2)
```
**Чому parent закриває обидва кінці pipe?** Інакше parent тримає відкритий write end, і `wc` ніколи не отримає EOF і не завершиться. Так само кожен child після `dup2` закриває оригінальні дескриптори pipe.

# Частина III. Signals

## Завдання 8. SIGINT (`signal_int.c`)

```bash
./signal_int          # натисніть Ctrl+C кілька разів
```
```text
^C
SIGINT received
^C
SIGINT received
^C
SIGINT received
```
Завершити програму: Ctrl+\ (SIGQUIT) або `kill -KILL PID` з іншого терміналу.

Ctrl+C надсилає foreground-процесу сигнал `SIGINT`. Власний handler замінює default поведінку (завершення). У handler використано `write()`, а не `printf()`, бо signal handler може виконатися **асинхронно** у будь-який момент, а `printf` не є async-signal-safe (`write` є).

## Завдання 9. Надсилання сигналу (`signal_parent_child.c`)

```bash
gcc -Wall -Wextra signal_parent_child.c -o signal_parent_child && ./signal_parent_child
gcc -Wall -Wextra -DCUSTOM_HANDLER signal_parent_child.c -o signal_parent_child_handler && ./signal_parent_child_handler
```
```text
Child: PID=884, waiting for a signal...
Parent: sending SIGTERM to child 884
Parent: child terminated by signal 15
```
і з handler:
```text
Child: PID=886, waiting for a signal...
Parent: sending SIGTERM to child 886
Child: SIGTERM received, exiting gracefully
Parent: child exited with status 0
```
(PID у вас будуть іншими.) За default поведінкою SIGTERM завершує child, і parent бачить `terminated by signal 15`. З власним handler child сам обробляє сигнал і завершується зі статусом 0.

`kill(pid, SIGTERM)` означає «надіслати сигнал SIGTERM», а не «негайно знищити процес»; результат залежить від disposition цього сигналу в процесі.

## Завдання 10. SIGKILL

```bash
./signal_int &           # запам'ятайте PID
kill -INT <PID>          # викликає handler, процес живе
kill -KILL <PID>         # процес завершується
```
**Чому не можна встановити handler для SIGKILL?** `SIGKILL` (як і `SIGSTOP`) не може бути перехоплений, ігнорований чи заблокований. Це гарантує, що ОС завжди має спосіб завершити процес, що вийшов з-під контролю.

# Частина IV. Mini-Shell (`mini_shell.c`)

```bash
gcc -Wall -Wextra mini_shell.c -o mini_shell && ./mini_shell
```

## Підтримувані можливості

| Можливість | Реалізація |
|---|---|
| prompt `myshell> ` | `printf` + `fflush`, читання `fgets` |
| зовнішні команди (`ls`, `ls -l`, `echo hello`, `pwd`) | `fork()` → `execvp()` → `waitpid()` |
| `exit` | built-in, завершує цикл shell (без fork) |
| `cd <dir>` | built-in через `chdir()` (без fork) |
| `cmd > file` | у child: `open` + `dup2` перед `execvp` |
| `cmd1 \| cmd2` | `pipe()` + 2 child + `dup2` |
| Ctrl+C | shell ігнорує `SIGINT`, child відновлює `SIG_DFL` |
| помилки | обробляються `fork`, `pipe`, `open`, `dup2`, `execvp`, `chdir`, `waitpid` |

Не використовуються `system()` та готові shell-парсери. Parser власний: токени розділяються пробілами/табами, спеціальні токени `|` та `>`, без лапок.

## Приклад сеансу

```text
myshell> pwd
/workspaces/labs-Arhitektura-Derehlazova/lab07-ipc-shell
myshell> /workspaces/labs-Arhitektura-Derehlazova/lab07-ipc-shell
myshell: /workspaces/labs-Arhitektura-Derehlazova/lab07-ipc-shell: Permission denied
myshell> ls
README.md      pipe_no_close     redirect_exec.c
dup2_output    pipe_no_close.c   redirected.txt
dup2_output.c  pipe_ok           signal_int
ls-output.txt  pipe_processes    signal_int.c
mini_shell     pipe_processes.c  signal_parent_child
mini_shell.c   pipeline          signal_parent_child.c
pipe_basic     pipeline.c        signal_parent_child_handler
pipe_basic.c   redirect_exec
myshell> ls -l
total 268
-rw-rw-rw- 1 codespace codespace 15860 Nov 22 05:48 README.md
-rwxrwxrwx 1 codespace codespace 16216 Nov 22 05:39 dup2_output
-rw-rw-rw- 1 codespace codespace   497 Nov 21 22:46 dup2_output.c
-rw-r--r-- 1 codespace codespace  1453 Nov 22 05:44 ls-output.txt
-rwxrwxrwx 1 codespace codespace 21712 Nov 22 05:39 mini_shell
-rw-rw-rw- 1 codespace codespace  8257 Nov 21 22:46 mini_shell.c
-rwxrwxrwx 1 codespace codespace 16224 Nov 22 05:39 pipe_basic
-rw-rw-rw- 1 codespace codespace   839 Nov 21 22:46 pipe_basic.c
-rwxrwxrwx 1 codespace codespace 16440 Nov 22 05:39 pipe_no_close
-rw-rw-rw- 1 codespace codespace  1655 Nov 21 22:46 pipe_no_close.c
-rwxrwxrwx 1 codespace codespace 16440 Nov 22 05:40 pipe_ok
-rwxrwxrwx 1 codespace codespace 16360 Nov 22 05:39 pipe_processes
-rw-rw-rw- 1 codespace codespace  1099 Nov 21 22:46 pipe_processes.c
-rwxrwxrwx 1 codespace codespace 16312 Nov 22 05:39 pipeline
-rw-rw-rw- 1 codespace codespace  1699 Nov 21 22:46 pipeline.c
-rwxrwxrwx 1 codespace codespace 16360 Nov 22 05:39 redirect_exec
-rw-rw-rw- 1 codespace codespace   983 Nov 21 22:46 redirect_exec.c
-rw-r--r-- 1 codespace codespace    22 Nov 22 05:44 redirected.txt
-rwxrwxrwx 1 codespace codespace 16192 Nov 22 05:39 signal_int
-rw-rw-rw- 1 codespace codespace   565 Nov 21 22:46 signal_int.c
-rwxrwxrwx 1 codespace codespace 16408 Nov 22 05:39 signal_parent_child
-rw-rw-rw- 1 codespace codespace  1690 Nov 21 22:46 signal_parent_child.c
-rwxrwxrwx 1 codespace codespace 16624 Nov 22 05:49 signal_parent_child_handler
myshell> echo hello
hello
myshell> cd /tmp
myshell> pwd
/tmp
myshell> echo hello > output.txt
myshell> cat output.txt
hello
myshell> ls -l | wc -l
12
myshell> nonexistent_command
myshell: nonexistent_command: No such file or directory
myshell> exit
```
Для довідки очікувана поведінка:
```text
myshell> pwd
/workspaces/...
myshell> cd /tmp
myshell> pwd
/tmp
myshell> echo hello > output.txt
myshell> cat output.txt
hello
myshell> ls -l | wc -l
<число>
myshell> nonexistent_command
myshell: nonexistent_command: No such file or directory
myshell> exit
```
Неіснуюча команда повідомляє про помилку, і shell продовжує працювати.

## Чому `cd` та `exit` мають бути built-in

Якщо виконувати `cd` через `fork()` + `chdir()` у child, **поточна директорія зміниться лише в child**, який одразу завершиться. Parent shell залишиться в старій директорії. Робочий каталог це властивість процесу, тому змінювати його має сам shell. Так само `exit` має завершити **сам shell**, а не child.
```text
Shell ─ fork ─► Child ─ chdir("/tmp") ─ exit      ✗ Shell лишився в старій директорії
Shell ─ chdir("/tmp")                             ✓ Shell змінив власну директорію
```

## Signals у mini-shell

```text
myshell> sleep 30
^C                  ← sleep завершується, shell лишається живим
myshell> echo alive
alive
```
Shell ігнорує `SIGINT` (`SIG_IGN`), тому Ctrl+C його не вбиває. Ігнорування **успадковується через `exec`**, тому в child перед `execvp` викликається `sigaction(SIGINT, SIG_DFL)`, і foreground-команда завершується за Ctrl+C. Це спрощена модель, повноцінний job control значно складніший.

`myshell> sleep 30
^C
myshell> echo alive
alive
myshell> exit`

## Тест Signal Behavior (онлайн)

Тест у курсі (`dist.krok.edu.ua`) виконується окремо в системі Moodle, у репозиторії його немає.

## Відповіді на контрольні запитання

- **Pipe:** односпрямований канал у ядрі для передавання байтів між процесами.
- **`pipefd[0]` / `pipefd[1]`:** кінець читання / кінець запису.
- **Чому закривати зайві кінці:** щоб напрямок був чітким і щоб читач міг отримати EOF.
- **Коли `read()` на pipe повертає 0:** коли pipe порожній і всі write ends закриті.
- **`dup2(old, new)`:** робить `new` копією `old` (закриваючи `new` перед цим).
- **Як працює `>`:** shell відкриває файл і за допомогою `dup2` підставляє його замість stdout перед `exec`.
- **Чому redirection працює без змін у програмі:** програма пише в дескриптор 1, а на що він вказує вирішує shell.
- **Pipeline:** stdout першого процесу з'єднується через pipe зі stdin другого.
- **Signal:** асинхронне повідомлення процесу від ядра або іншого процесу.
- **`SIGINT`:** сигнал переривання (Ctrl+C). **`SIGTERM`:** запит на завершення. **`SIGKILL`:** примусове завершення, його не можна перехопити.
- **Чому `write()` у handler:** async-signal-safe, на відміну від `printf`.
- **Чому `cd` built-in:** змінює директорію самого shell.
- **Чому shell ігнорує SIGINT:** щоб Ctrl+C не вбивав shell.
- **Чому child відновлює `SIG_DFL`:** ігнорування успадковується через `exec`, а команда має завершуватися за Ctrl+C.
- **Чому `exec` не повертається:** образ процесу замінюється; повернення означає помилку.
- **Чому в child після невдалого `exec` використовується `_exit()`:** не виконувати обробники та скидання буферів, успадкованих від parent.

## Висновок

Pipe з'єднує процеси через дескриптори, EOF залежить від закриття всіх write ends, `dup2` змінює, на що вказує stdout/stdin, і саме це лежить в основі `>` та `|`. Сигнали по-різному діють залежно від disposition, а `SIGKILL` не можна перехопити. Mini-shell поєднує всі ці механізми: `fork`, `exec`, `wait`, `pipe`, `dup2`, `chdir` та обробку `SIGINT`.
