# Lab 4. Процеси, системні виклики та `fork()` / `exec()` / `wait()`

**Середовище:** GitHub Codespaces, Ubuntu, GCC, `strace`, `ps`, `/proc`.
Executable-файли в репозиторій не додаються. Усі програми компілюються так: `gcc -Wall -Wextra file.c -o file`.

## Файли

`process_info.c`, `hello.c`, `output.c`, `fork_basic.c`, `fork_memory.c`, `wait_basic.c`, `exec_basic.c`, `fork_exec.c`, `launcher.c`, `states.c`, `zombie.c`, `process_runner.c`, а також додатковий `fork_count.c` (експеримент із кількістю процесів).

> Якщо `strace` не встановлений: `sudo apt-get update && sudo apt-get install -y strace`.

## Завдання 1–2. Program та Process (`process_info.c`)

Запустіть програму у **трьох терміналах** одночасно (або `./process_info &` тричі):
```bash
./process_info
```
```text
PID:  39534
PPID: 1044
[2] 39545
PID:  39545
PPID: 1044
[3] 39556
PID:  39556
PPID: 1044
```

| Instance | PID | PPID |
|---|---|---|
| 1 | `39534` | `1044` |
| 2 | `39545` | `1044` |
| 3 | `39556` | `1044` |

PID у кожного запуску різний, PPID однаковий (це PID shell, з якого запущено програми). **Executable це файл, process це його запущений екземпляр.** З одного executable можна отримати кілька процесів із різними PID.

## Завдання 3. `/proc/PID`

Поки `process_info` працює (30 секунд), в іншому терміналі:
```bash
ls /proc/<PID>
cat /proc/<PID>/status | head -n 12
ls -l /proc/<PID>/exe
cat /proc/<PID>/cmdline | tr '\0' ' '
```
```text
[1]   Done                    ./process_info
[2]-  Done                    ./process_info
[3]+  Done                    ./process_info
[1] 41361
PID:  41361
PPID: 1044
arch_status         ksm_stat       projid_map
attr                latency        root
autogroup           limits         sched
auxv                loginuid       schedstat
cgroup              map_files      sessionid
clear_refs          maps           setgroups
cmdline             mem            smaps
comm                mountinfo      smaps_rollup
coredump_filter     mounts         stack
cpu_resctrl_groups  mountstats     stat
cpuset              net            statm
cwd                 ns             status
environ             numa_maps      syscall
exe                 oom_adj        task
fd                  oom_score      timens_offsets
fdinfo              oom_score_adj  timers
gid_map             pagemap        timerslack_ns
io                  patch_state    uid_map
ksm_merging_pages   personality    wchan
Name:   process_info
Umask:  0022
State:  S (sleeping)
Tgid:   41361
Ngid:   0
Pid:    41361
PPid:   1044
TracerPid:      0
Uid:    1000    1000    1000    1000
Gid:    1000    1000    1000    1000
FDSize: 256
Groups: 986 987 988 989 990 991 992 993 994 995 996 1000 1001 
lrwxrwxrwx 1 codespace codespace 0 Nov 22 01:53 /proc/41361/exe
```
`/proc/PID` це віртуальна директорія, через яку ядро показує інформацію про процес: ім'я, стан (`State: S (sleeping)`), PID/PPID, використання пам'яті, відкриті файли (`fd/`), карту пам'яті (`maps`), посилання на executable (`exe`), аргументи (`cmdline`).

## Завдання 4–5. System calls (`hello.c`, `output.c`)

```bash
strace ./hello
```
```text
write(1, "Hello from process!\n", 20Hello from process!
)   = 20
exit_group(0)                           = ?
+++ exited with 0 +++
```
`printf()` це функція бібліотеки C; усередині вона викликає системний виклик **`write()`**, який просить ядро записати байти у файловий дескриптор 1 (stdout). `strace` показує ці виклики до ядра, їхні аргументи та значення, що повертаються.

**Library function та system call.** Library function (`printf`) виконується в user space, форматує рядок, буферизує вивід. System call (`write`) це перехід у kernel mode: запит до ОС на виконання привілейованої дії. `printf` викликає `write` зрештою, але не завжди одразу (через буфер).

`output.c` друкує двома способами: `printf` та `write`. При виводі в термінал порядок збігається з порядком у коді; при перенаправленні (`./output | cat`) `write()` може з'явитися раніше за `printf()`, бо `printf` тримає дані в буфері до завершення програми.
```text
Output using printf
Output using write
Output using write
Output using printf
```

## Завдання 6. Перший `fork()` (`fork_basic.c`)

```bash
./fork_basic
```
```text
Before fork: PID=43597
PARENT: PID=43597, child PID=43598, fork returned=43598
CHILD:  PID=43598, PPID=43597, fork returned=0
```

- Після одного успішного `fork()` існує **2 процеси**: parent і child.
- **Return values:** у child `fork()` повертає `0`, у parent повертає PID child, при помилці повертає `-1`.
- **Чому в child 0?** Child завжди може отримати PID parent через `getppid()`, а parent потрібно знати PID child, тому саме його повертають parent. Нуль не може бути PID дійсного процесу, тому це зручний маркер «я child».
- **Порядок виводу не гарантований:** після `fork()` обидва процеси готові до виконання, і планувальник ОС може запустити будь-який першим. Якщо parent завершується раніше, child може показати інший PPID (наприклад, 1 або PID subreaper-процесу).
- `fflush(stdout)` перед `fork()` потрібен, щоб текст із буфера не був скопійований у child і не надрукувався двічі.

## Завдання 7. `fork()` та пам'ять (`fork_memory.c`)

```bash
./fork_memory
```
```text
Before fork: value=100, address=0x5b6f40bfb010
PARENT: PID=43826 value=300 address=0x5b6f40bfb010
CHILD:  PID=43827 value=200 address=0x5b6f40bfb010
```

**Чи мають parent і child спільну змінну?** Ні. Після `fork()` у child створюється копія адресного простору (copy-on-write), тому зміна `value` у child не впливає на parent. **Однакові virtual addresses** (`0x...1010`) не означають спільної пам'яті: це адреси у **різних** віртуальних адресних просторах, які ядро відображає на різні фізичні сторінки після запису.

## Завдання 8. `strace` для `fork()`

```bash
strace -f -e trace=process ./fork_basic
```
```text
Before fork: PID=44407
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLDstrace: Process 44408 attached
, child_tidptr=0x7ac32fbd3a10) = 44408
PARENT: PID=44407, child PID=44408, fork returned=44408
[pid 44407] exit_group(0)               = ?
[pid 44407] +++ exited with 0 +++
CHILD:  PID=44408, PPID=44407, fork returned=0
exit_group(0)                           = ?
+++ exited with 0 +++
```
Сучасна бібліотека C реалізує `fork()` через системний виклик `clone()`. У виводі з'являється `clone(...)= <PID child>` у parent і повідомлення про завершення обох процесів `exit_group`.

## Завдання 9. `wait()` (`wait_basic.c`)

```bash
./wait_basic
```
```text
PARENT: waiting for child 46698
CHILD: working...
CHILD: finished
PARENT: child finished
PARENT: child exit status = 42
```
Очікуваний результат: `CHILD: working...`, `CHILD: finished`, `PARENT: waiting for child <PID>`, `PARENT: child finished`, `PARENT: child exit status = 42`.

`wait()` блокує parent, доки child не завершиться, і повертає його PID та статус. **Exit status** це код, з яким завершився процес (`return 42` у child), який parent отримує через `WIFEXITED(status)` / `WEXITSTATUS(status)`. Без `wait()` parent не отримує результат, а завершений child лишається zombie.

## Завдання 10. `exec()` (`exec_basic.c`)

```bash
./exec_basic
```
```text
Before exec: PID=46391
Hello from exec!
```
`exec()` **замінює** образ програми (код, дані, stack, heap) поточного процесу на нову програму. **Новий процес не створюється**, а **PID не змінюється**. Рядок, який стоїть у коді після `execlp`, не виконується, тому що старий код більше не існує (він виконується лише якщо `exec` завершився помилкою).

## Завдання 11. `fork()` + `exec()` (`fork_exec.c`)

```bash
./fork_exec
```
```text
PARENT: PID=45743
PARENT: created child PID=45744
CHILD before exec: PID=45744
Program started by child
PARENT: child completed
```
Схема: parent викликає `fork()` → створюється child → child викликає `exec()` і стає іншою програмою (`echo`) → parent чекає через `wait()` і продовжує після завершення child.

```text
parent ── fork() ──► child ── exec(echo) ──► (нова програма) ── exit
   │                                                         │
   └──────────────────── wait() ◄────────────────────────────┘
```
Саме так shell запускає команди.

## Завдання 12. Launcher (`launcher.c`)

```bash
./launcher date
./launcher whoami
./launcher uname
```
```text
Sun Nov 22 02:06:09 UTC 2026
Child exit status: 0
Started child PID=47018
codespace
Child exit status: 0
Started child PID=47030
Linux
Child exit status: 0
```
`launcher` читає назву програми, робить `fork()`, у child запускає її через `exec()`, parent виконує `waitpid()` і показує exit status. Це ще не shell, але основа його роботи.

## Завдання 13. Process states (`states.c`)

```bash
./states &
ps -o pid,stat,cmd -p <PID>
cat /proc/<PID>/status | grep State
```
```text
[1] 47321
PID=47321
    PID STAT CMD
  47321 S    ./states
State:  S (sleeping)
```
Під час `sleep(60)` процес має стан **S** (sleeping, Waiting), тобто чекає на подію (таймер) і не споживає CPU.

- **Running (R):** процес виконується на CPU зараз.
- **Ready:** процес готовий, але чекає, коли планувальник дасть йому CPU (в Linux теж відображається як `R`).
- **Waiting (S / D):** процес чекає на подію (час, введення-виведення) і не може виконуватись.

## Завдання 14. Zombie (`zombie.c`)

```bash
./zombie &
ps -o pid,ppid,stat,cmd --ppid <PID_parent>
```
```text
[1] 48080
PARENT: PID=48080, child=48083
PARENT: sleeping without wait()
CHILD: PID=48083, exiting now
    PID    PPID STAT CMD
  48083   48080 Z    [zombie] <defunct>
```
Child завершується одразу, а parent 60 секунд спить і не викликає `wait()`. Ядро зберігає запис про child (PID, exit status), доки parent його не забере. У `ps` такий процес показується зі станом **Z** (`<defunct>`).

**Zombie це завершений процес**, від якого лишився лише запис у таблиці процесів з exit status. Він **не виконує інструкцій** і не використовує CPU чи пам'ять процесу, але займає PID. Лікується викликом `wait()` у parent (або завершенням parent).

## Завдання 15. `fork()` і кількість процесів (`fork_count.c`)

**Прогноз:** після `fork(); fork();` буде **4** процеси, тому `printf` виконається **4 рази**.

```bash
./fork_count
```
```text
Child created, PID: 48732
Child PID: 48732
Child PPID: 48731
Executing: nonexistent_command
exec nonexistent_command failed: No such file or directory
Child 48732 completed
Exit status: 127
Parent continues
```

Process tree:
```text
P (оригінал)
├── C1 (перший fork)
│    └── C3 (другий fork у C1)
└── C2 (другий fork у P)
```
Перший `fork()` дає 2 процеси; обидва виконують другий `fork()`, і виходить 4.

**Додатково:** для трьох `fork()` буде 2^3 = 8 процесів (n послідовних `fork()` дають 2^n).

## Фінальне завдання. `process_runner.c`

```bash
gcc -Wall -Wextra process_runner.c -o process_runner
./process_runner date
./process_runner false
./process_runner nonexistent_command
```
```text
execve("./process_runner", ["./process_runner", "date"], 0x7ffd1a441730 /* 93 vars */) = 0
Parent PID: 49013
Creating child...
clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLDstrace: Process 49014 attached
, child_tidptr=0x7cdf1bdc0a10) = 49014
Child created, PID: 49014
Child PID: 49014
[pid 49013] wait4(49014, Child PPID: 49013
Executing: date
 <unfinished ...>
[pid 49014] execve("/vscode/bin/linux-x64/8a14f0cc1b08d531eb83255ea3764ae22aa932b5/bin/remote-cli/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/.local/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/rubies/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/.dotnet/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/nvm/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/.php/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/.python/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/java/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/.ruby/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/home/codespace/.local/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/python/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/py-utils/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/jupyter/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/oryx/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/go/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/go/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/sdkman/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/sdkman/candidates/java/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/sdkman/candidates/gradle/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/sdkman/candidates/maven/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/sdkman/candidates/ant/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/share/rbenv/shims/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/share/rbenv/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/rubies/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/php/current/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/opt/conda/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/share/nvm/versions/node/v24.22.2/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/hugo/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/sbin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/local/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/sbin/date", ["date"], 0x7fff77166070 /* 93 vars */) = -1 ENOENT (No such file or directory)
[pid 49014] execve("/usr/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = 0
Sun Nov 22 02:09:59 UTC 2026
[pid 49014] exit_group(0)               = ?
[pid 49014] +++ exited with 0 +++
<... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 49014
--- SIGCHLD {si_signo=SIGCHLD, si_code=CLD_EXITED, si_pid=49014, si_uid=1000, si_status=0, si_utime=0, si_stime=0} ---
Child 49014 completed
Exit status: 0
Parent continues
exit_group(0)                           = ?
+++ exited with 0 +++
```

Для неіснуючої команди child виводить помилку `exec ... failed: No such file or directory` і завершується зі статусом `127`, parent це виявляє й продовжує роботу. Обробляються помилки `fork`, `exec` (у child) та `waitpid` (повторний виклик при `EINTR`).

### `strace`

```bash
strace -f -e trace=process ./process_runner date
```
Три фрагменти (вставте з вашого виводу):
```text
створення процесу:       clone(child_stack=NULL, flags=CLONE_CHILD_CLEARTID|CLONE_CHILD_SETTID|SIGCHLD, child_tidptr=0x7cdf1bdc0a10) = 49014
запуск нової програми:   [pid 49014] execve("/usr/bin/date", ["date"], 0x7fff77166070 /* 93 vars */) = 0
завершення/очікування:   <... wait4 resumed>[{WIFEXITED(s) && WEXITSTATUS(s) == 0}], 0, NULL) = 49014
```
Ланцюжок: наш C-код → `fork/exec/waitpid` (POSIX API) → системні виклики `clone` / `execve` / `wait4` → ядро ОС.

## Відповіді на контрольні питання

- **Program та process:** program це файл з кодом на диску, process це його виконання з власним станом і адресним простором.
- **PID:** унікальний ідентифікатор процесу.
- **PPID:** PID батьківського процесу.
- **Чому один executable має кілька PID:** з одного файла можна запустити кілька процесів.
- **`/proc/PID`:** віртуальна директорія з інформацією про процес (стан, пам'ять, відкриті файли).
- **System call:** запит програми до ядра ОС на привілейовану операцію.
- **Function call та system call:** function call це звичайний виклик у user space; system call це перехід у kernel mode.
- **`strace`:** показує системні виклики процесу та їхні результати.
- **`fork()`:** створює новий процес (копію поточного).
- **Скільки процесів після одного fork:** 2.
- **Чому 0 в child:** позначає, що це child (і 0 не є дійсним PID).
- **Що отримує parent:** PID child (або -1 при помилці).
- **Спільна `value`?** Ні, у кожного процесу своя копія.
- **Однакові virtual addresses:** це адреси різних віртуальних просторів, які відображаються на різну фізичну пам'ять.
- **`exec()`:** замінює програму в поточному процесі.
- **Чи створює `exec()` новий процес?** Ні.
- **PID після exec:** не змінюється.
- **Навіщо `wait()`:** щоб дочекатися child, отримати його статус і не залишити zombie.
- **Exit status:** код завершення процесу (0 це успіх).
- **Zombie:** завершений процес, статус якого ще не забраний parent.
- **Чи виконує zombie інструкції?** Ні.
- **Ready / Running / Waiting:** Ready (готовий, чекає CPU), Running (виконується на CPU), Waiting (чекає події: введення-виведення, таймера).
- **Порядок parent/child:** визначається планувальником ОС, його не можна передбачити.
- **Зв'язок із shell:** shell робить `fork()`, у child `exec()` команди, а сам чекає `wait()`.

## Висновок

Експериментально встановлено: `fork()` створює копію процесу з незалежною пам'яттю (повертає 0 у child і PID child у parent), `exec()` замінює програму в тому самому процесі без зміни PID, `wait()` забирає exit status і запобігає zombie. Комбінація `fork → exec → wait` лежить в основі роботи shell, а `strace` показує відповідні системні виклики (`clone`, `execve`, `wait4`).
