int main() {
    int i;
    i = 0;
    again:
    i++;
    if (i < 3) goto again;
    return 0;
}
