void encode_int_as_char(int num, char *buf, int length){
    int i;
    for (i = 0; i < length; i++){
        buf[i] = (char)(num >> ((8 * (length - i - 1)) & 0xFF));
    }
}

int decode_int_from_char(char *enc, int length){
    int i, num, cur;

    num = 0;
    for (i = 0; i < length; i++){
        cur = (unsigned char) enc[i] << (8 * (length - i - 1));
        num += (int) cur;
    }

    return num;
}

