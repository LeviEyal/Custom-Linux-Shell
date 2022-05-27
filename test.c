#include "stdlib.h"

int main() {
    setenv("name", "/bin", 1);
    char* token = getenv("name");
    printf("%s\n", token);
}