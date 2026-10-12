int main() {
    int i, s;
    s = 0;
    for (int j = 0; j < 3; j++) { s += j; }
    while (s > 0) { s--; }
    do { s++; } while (s < 2);
    return 0;
}
