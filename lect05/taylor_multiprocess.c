#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <math.h>
#include <errno.h>
#include <string.h>

static long double term_exp(long double x, int k) {
    if (k == 0) return 1.0L;
    long double t = 1.0L;
    for (int i = 1; i <= k; ++i) {
        t *= x / (long double)i;
    }
    return t;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s x n_terms n_procs\n", argv[0]);
        fprintf(stderr, "Example: %s 1.0 20 4\n", argv[0]);
        return 1;
    }
    
    long double x = strtold(argv[1], NULL);
    int n_terms = atoi(argv[2]);  
    int n_procs = atoi(argv[3]);
    
    if (n_terms <= 0 || n_procs <= 0) {
        fprintf(stderr, "n_terms와 n_procs는 양수여야 합니다.\n");
        return 1;
    }
    if (n_procs > n_terms) n_procs = n_terms; 
  
    int (*ranges)[2] = calloc(n_procs, sizeof(int[2]));
    if (!ranges) {
        perror("calloc");
        return 1;
    }
    
    int base = n_terms / n_procs;
    int rem  = n_terms % n_procs;
    int idx = 0;
    for (int p = 0; p < n_procs; ++p) {
        int len = base + (p < rem ? 1 : 0);
        ranges[p][0] = idx;
        ranges[p][1] = idx + len;
        idx += len;
    }
    
    int (*pipes)[2] = calloc(n_procs, sizeof(int[2]));
    if (!pipes) {
        perror("calloc");
        free(ranges);
        return 1;
    }
    
    for (int p = 0; p < n_procs; ++p) {
        if (pipe(pipes[p]) == -1) {
            perror("pipe");
            free(ranges);
            free(pipes);
            return 1;
        }
    }
    
   
    for (int p = 0; p < n_procs; ++p) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            free(ranges);
            free(pipes);
            return 1;
        }
        
        if (pid == 0) {
            
            for (int q = 0; q < n_procs; ++q) {
                close(pipes[q][0]);
                if (q != p) close(pipes[q][1]);   
            }
            
            int start = ranges[p][0];
            int end   = ranges[p][1];
            long double partial = 0.0L;
            for (int k = start; k < end; ++k) {
                partial += term_exp(x, k);
            }
            
            double send_val = (double)partial;
            if (write(pipes[p][1], &send_val, sizeof(send_val)) != sizeof(send_val)) {
                fprintf(stderr, "child %d write error: %s\n", p, strerror(errno));
            }
            close(pipes[p][1]);
            _exit(0);
        }
        
       
        close(pipes[p][1]); 
    }
    
    
    long double total = 0.0L;
    for (int p = 0; p < n_procs; ++p) {
        double recv_val = 0.0;
        ssize_t n = read(pipes[p][0], &recv_val, sizeof(recv_val));
        if (n == (ssize_t)sizeof(recv_val)) {
            total += (long double)recv_val;
        } else {
            fprintf(stderr, "parent read error from child %d\n", p);
        }
        close(pipes[p][0]);
    }
    
    
    for (int p = 0; p < n_procs; ++p) {
        int status = 0;
        wait(&status);
    }
    
    long double truev = expl(x);           
    long double err   = fabsl(truev - total);
    
    printf("=== e^x Taylor 병렬 계산 ===\n");
    printf("x = %.17Lg, terms = %d, procs = %d\n", x, n_terms, n_procs);
    printf("approx  = %.17Lg\n", total);
    printf("lib exp = %.17Lg\n", truev);
    printf("abs err = %.17Lg\n", err);
    
    free(ranges);
    free(pipes);
    return 0;
}
