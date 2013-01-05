#include <cstdint>
#include <algorithm>
#include <unistd.h>

int main() {
    int32_t n, *arr;
    while (read(STDIN_FILENO, &n, sizeof(n)) == sizeof(n)) {
        arr = new int32_t[n];
        fread(arr, 1, sizeof(arr[0])*n, stdin);
        std::sort(arr, arr+n);
        write(STDOUT_FILENO, arr, sizeof(arr[0])*n);
        delete arr;
    }
    return 0;
}

