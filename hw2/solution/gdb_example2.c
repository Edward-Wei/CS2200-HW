#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setint(int* ip, int i) {
    *ip = i;
} 

void write_message(char *message) {
    char buffer[100];

    memset(buffer, '\0', 100);
    strcpy(buffer, message);
    printf("%s\n", buffer);
}

int main() {
    int a;
    char message[] = "Look, this seems like an innocent message!";

    setint(&a, 10);
    printf("%d\n", a);

    write_message(message);

    int *b;
    setint(b, 20);
    printf("%ld\n", *b);
    printf("This may or may not have crashed. You are lucky if you see this.");
    return 0;
}
