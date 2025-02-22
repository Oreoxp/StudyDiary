/* 
 * tsh - A tiny shell program with job control
 * 
 * <Put your name and login ID here>
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* Misc manifest constants */
#define MAXLINE 1024   /* max line size */
#define MAXARGS 128    /* max args on a command line */
#define MAXJOBS 16     /* max jobs at any point in time */
#define MAXJID 1 << 16 /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;   /* defined in libc */
char prompt[] = "tsh> "; /* command line prompt (DO NOT CHANGE) */
int verbose = 0;         /* if true, print additional output */
int nextjid = 1;         /* next job ID to allocate */
char sbuf[MAXLINE];      /* for composing sprintf messages */

struct job_t
{                          /* The job struct */
    pid_t pid;             /* job PID */
    int jid;               /* job ID [1, 2, ...] */
    int state;             /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE]; /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */

/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv);
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs);
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid);
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid);
int pid2jid(pid_t pid);
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv)
{
    printf("main : start\n");
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* 发出提示（默认） */

    /* 将stderr重定向到stdout
    *（以便驱动程序将在连接到stdout的管道上获得所有输出）  */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF)
    {
        switch (c)
        {
        case 'h': /* print help message */
            usage();
            break;
        case 'v': /* emit additional diagnostic info */
            verbose = 1;
            break;
        case 'p':            /* don't print a prompt */
            emit_prompt = 0; /* handy for automatic testing */
            break;
        default:
            usage();
        }
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT, sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler); /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler); /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler);

    /* Initialize the job list */
    initjobs(jobs);

    printf("main : read/eval loop\n");
    /* 执行shell的 read/eval loop */
    while (1) {
        /* Read command line */
        if (emit_prompt) {
            printf("%s", prompt);
            fflush(stdout);
        }
        if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin)) {
            app_error("fgets error");
        }
        if (feof(stdin)) { /* End of file (ctrl-d) */
            fflush(stdout);
            exit(0);
        }

        /* Evaluate the command line */
        eval(cmdline);
        fflush(stdout);
        fflush(stdout);
    }

    printf("main : exit\n");
    exit(0); /* control never reaches here */
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * 如果用户请求了内置命令（退出，作业，bg或fg），则立即执行它。 否则，请派生一个子进程，并在该子进程的上下文中运行该作业。 
 * 如果作业在前台运行，请等待其终止然后返回。 
 * 
 * 注意：每个子进程必须具有唯一的进程组ID，以便当我们在键盘上键入ctrl-c（ctrl-z）时，
 * 我们的后台子进程不会从内核接收SIGINT（SIGTSTP）。 
*/
void eval(char *cmdline)
{
    printf("eval : start\n");
    char *argv[MAXARGS];
    char cmdline_buf[MAXLINE];
    int bg;
    int state;
    pid_t pid;
    sigset_t mask_all, mask_one, prev;

    strcpy(cmdline_buf, cmdline);
    bg = parseline(cmdline_buf, argv);

    // 没有参数就退出
    // 其实可以加一个判断参数数量是否正确的语句，比较完整
    if (argv[0] == NULL)
        return;

    // 如果不是内置的命令，则执行
    if (!builtin_cmd(argv)) {
        printf("eval : exec cmd\n");
        // 在函数内部加阻塞列表，不然之后可能会出现不痛不痒的bug
        sigfillset(&mask_all);
        sigemptyset(&mask_one);
        sigaddset(&mask_one, SIGCHLD);

        // 为了避免父进程运行到addjob之前子进程就退出了，所以
        // 在fork子进程前阻塞sigchld信号，addjob后解除
        sigprocmask(SIG_BLOCK, &mask_one, &prev);

        if ((pid = fork()) == 0) {//child
            printf("eval : fork->child start\n");

            // 子进程继承了父进程的阻塞向量，也要解除阻塞，
            // 避免收不到它本身的子进程的信号
            sigprocmask(SIG_SETMASK, &prev, NULL);

            // 改进程组与自己pid一样
            if (setpgid(0, 0) < 0) {
                perror("SETPGID ERROR");
                exit(0);
            }

            // 正常运行execve函数会替换内存，不会返回/退出，所以必须要加exit，
            // 否则会一直运行下去，子进程会开始运行父进程的代码
            if (execve(argv[0], argv, environ) < 0) {
                printf("%s: Command not found\n", argv[0]);
                exit(0);
            }
        } else { //parent
            printf("eval : fork->parent start\n");

            state = bg ? BG : FG;

            // 依然是加塞，阻塞所有信号
            sigprocmask(SIG_BLOCK, &mask_all, NULL);
            addjob(jobs, pid, state, cmdline);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }

        printf("eval : exec end\n");

        // 后台则打印，前台则等待子进程结束
        if (!bg) {
            waitfg(pid);
        } else {
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        }

        // 后面又想了想，打印后台的时候其实走的是全局变量，也应该加塞才对，
        // 应该是所有的用到全局变量的都应该加塞，但是懒，不改了，知道就行
    }

    printf("eval : return\n");
    return;
}

/* 
 * parseline - 解析命令行并构建argv数组。 
 * 
 * 用单引号引起来的字符被视为单个参数。 
 * 如果用户请求了BG作业，则返回true；
 * 如果用户请求了FG作业，则返回false。  
 */
int parseline(const char *cmdline, char **argv)
{
    printf("parseline: start\n");
    static char array[MAXLINE]; /* 保存命令行的本地副本 */
    char *buf = array;          /* 遍历命令行的ptr*/
    char *delim;                /* 指向第一个空格定界符 */
    int argc;                   /* 参数个数 */
    int bg;                     /* 后台工作？ */

    strcpy(buf, cmdline);
    buf[strlen(buf) - 1] = ' ';   /* 用空格替换结尾的“ \ n”  */
    while (*buf && (*buf == ' ')) /* 忽略前导空格 */
        buf++;

    /* 建立argv清单  */
    argc = 0;
    if (*buf == '\'') {
        buf++;
        delim = strchr(buf, '\'');
    } else {
        delim = strchr(buf, ' ');
    }

    while (delim) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' ')) /* 忽略空格 */
            buf++;

        if (*buf == '\'') {
            buf++;
            delim = strchr(buf, '\'');
        } else {
            delim = strchr(buf, ' ');
        }
    }
    argv[argc] = NULL;

    if (argc == 0) /* ignore blank line */
        return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc - 1] == '&')) != 0) {
        argv[--argc] = NULL;
    }

    printf("parseline: return\n");
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv)
{
    // 判断是不是内置函数，不是就返回，注意内置命令有要继续操作的一定
    // 要返回1，不是内置函数就是0

    printf("builtin_cmd: start\n");
    printf(argv[0]);

    if (!strcmp(argv[0], "quit")) {
        exit(0);
    } else if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    } else if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }
    else if (!strcmp(argv[0], "&")) { // 对单独的&不处理
        return 1;
    }
    printf("builtin_cmd: end\n");
    return 0; /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv)
{
    printf("do_bgfg : start\n");
    // 没有参数，其实应该也加上判断参数个数的语句才比较完整
    if (argv[1] == NULL)
    {
        printf("%s command requires PID or %%jobid argument\n", argv[0]);
        return;
    }

    struct job_t *job;
    int id;

    // bg %5 和bg 5不一样,一个是对一个作业操作，另一个是对进程操作，
    // 而作业代表了一个进程组。

    // 要根据tshref的样例输出看有多少种情况

    // 读到jid
    if (sscanf(argv[1], "%%%d", &id) > 0)
    {
        job = getjobjid(jobs, id);
        if (job == NULL)
        {
            printf("%%%d: No such job\n", id);
            return;
        }
    }
    else if (sscanf(argv[1], "%d", &id) > 0)
    { // 读到pid
        job = getjobpid(jobs, id);
        if (job == NULL)
        {
            printf("(%d): No such process\n", id);
            return;
        }
    }
    else
    { // 格式错误
        printf("%s: argument must be a PID or %%jobid\n", argv[0]);
        return;
    }
    if (!strcmp(argv[0], "bg"))
    {   // 因为子进程单独成组，所以kill很方便
        // 进程组是负数pid，发送信号并更改状态
        kill(-(job->pid), SIGCONT);
        job->state = BG;
        printf("[%d] (%d) %s", job->jid, job->pid, job->cmdline);
    }
    else
    {
        // 如果fg后台进程，那么将它的状态转为前台进程，然后等待它终止
        kill(-(job->pid), SIGCONT);
        job->state = FG;
        waitfg(job->pid);
    }

    printf("do_bgfg : end\n");
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    printf("waitfg : start\n");
    // 进程回收不需要做，只要等待前台进程就行
    sigset_t mask_temp;
    sigemptyset(&mask_temp);
    // 设定不阻塞任何信号
    // 其实可以直接sleep显式等待信号
    while (fgpid(jobs) > 0)
        sigsuspend(&mask_temp);
    printf("waitfg : end\n");
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig)
{
    printf("sigchld_handler : start\n");
    int olderrno = errno; // 保存旧errno
    pid_t pid;
    int status;
    sigset_t mask_all, prev;

    sigfillset(&mask_all); // 设置全阻塞
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0)
    {
        // WNOHANG | WUNTRACED 是立即返回
        // 用WIFEXITED(status)，WIFSIGNALED(status)，WIFSTOPPED(status)等来补获终止或者
        // 被停止的子进程的退出状态。
        if (WIFEXITED(status)) // 正常退出 delete
        {
            sigprocmask(SIG_BLOCK, &mask_all, &prev);
            deletejob(jobs, pid);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
        else if (WIFSIGNALED(status)) // 信号退出 delete
        {
            struct job_t *job = getjobpid(jobs, pid);
            sigprocmask(SIG_BLOCK, &mask_all, &prev);
            printf("Job [%d] (%d) terminated by signal %d\n", job->jid, job->pid, WTERMSIG(status));
            deletejob(jobs, pid);
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
        else // 停止 只修改状态就行
        {
            struct job_t *job = getjobpid(jobs, pid);
            sigprocmask(SIG_BLOCK, &mask_all, &prev);
            printf("Job [%d] (%d) stopped by signal %d\n", job->jid, job->pid, WSTOPSIG(status));
            job->state = ST;
            sigprocmask(SIG_SETMASK, &prev, NULL);
        }
    }
    errno = olderrno; // 恢复
    printf("sigchld_handler : end\n");
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig)
{
    printf("sigint_handler : start\n");
    // 向子进程发送信号即可
    int olderrno = errno;
    pid_t pid = fgpid(jobs);
    if (pid != 0)
        kill(-pid, sig);
    errno = olderrno;

    printf("sigint_handler : end\n");
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig)
{
    printf("sigtstp_handler : start\n");

    // 向子进程发送信号即可
    int olderrno = errno;
    pid_t pid = fgpid(jobs);
    if (pid != 0)
        kill(-pid, sig);
    errno = olderrno;

    printf("sigtstp_handler : end\n");
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job)
{
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs)
{
    int i, max = 0;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid > max)
            max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == 0)
        {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].jid = nextjid++;
            if (nextjid > MAXJOBS)
                nextjid = 1;
            strcpy(jobs[i].cmdline, cmdline);
            if (verbose)
            {
                printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
        }
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid == pid)
        {
            clearjob(&jobs[i]);
            nextjid = maxjid(jobs) + 1;
            return 1;
        }
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].state == FG)
            return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid)
{
    int i;

    if (pid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
            return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid)
{
    int i;

    if (jid < 1)
        return NULL;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid)
{
    int i;

    if (pid < 1)
        return 0;
    for (i = 0; i < MAXJOBS; i++)
        if (jobs[i].pid == pid)
        {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs)
{
    int i;

    for (i = 0; i < MAXJOBS; i++)
    {
        if (jobs[i].pid != 0)
        {
            printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
            switch (jobs[i].state)
            {
            case BG:
                printf("Running ");
                break;
            case FG:
                printf("Foreground ");
                break;
            case ST:
                printf("Stopped ");
                break;
            default:
                printf("listjobs: Internal error: job[%d].state=%d ",
                       i, jobs[i].state);
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}
/******************************
 * end job list helper routines
 ******************************/

/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void)
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler)
{
    struct sigaction action, old_action;

    action.sa_handler = handler;
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
        unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig)
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}
