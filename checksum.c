unsigned short calc_checksum(unsigned char *header, int length) {
    unsigned int sum;
    unsigned short *ptr = (unsigned short *)header;
    int i;

    sum = 0;
    ptr = (unsigned short *)header;

    for (i = 0; i < length; i += 2) {
        sum += (*ptr);
        if (sum & 0x80000000) {
            sum = (sum & 0xFFFF) + (sum >> 16);
        }
        ptr++;
    }

    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    return (~sum);
}

unsigned short update_checksum(unsigned short checksum, unsigned short original_value, unsigned short new_value) {
    return checksum + (new_value - original_value);
}