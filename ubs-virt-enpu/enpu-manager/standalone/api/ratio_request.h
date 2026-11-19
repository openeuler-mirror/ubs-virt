#ifndef ENPU_RATIO_REQUEST_H
#define ENPU_RATIO_REQUEST_H
#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static inline void ratio_skip_space(const char **p)
{
    while (**p == ' ' || **p == '\t' || **p == '\r' || **p == '\n')
        ++*p;
}

/* This endpoint accepts exactly two numeric fields, in either order. */
static inline int parse_ratio_request(const char *p, int max_cards, int *phy_id, double *ratio)
{
    if (p == NULL || phy_id == NULL || ratio == NULL)
        return -1;
    unsigned seen = 0;
    ratio_skip_space(&p);
    if (*p++ != '{')
        return -1;
    for (int field = 0; field < 2; field++) {
        ratio_skip_space(&p);
        unsigned bit;
        if (strncmp(p, "\"phy_id\"", 8) == 0) {
            bit = 1;
            p += 8;
        } else if (strncmp(p, "\"ratio\"", 7) == 0) {
            bit = 2;
            p += 7;
        } else
            return -1;
        if (seen & bit)
            return -1;
        seen |= bit;
        ratio_skip_space(&p);
        if (*p++ != ':')
            return -1;
        ratio_skip_space(&p);
        const char *start = p;
        if (*p == '-')
            p++;
        if (*p == '0')
            p++;
        else {
            if (*p < '1' || *p > '9')
                return -1;
            while (isdigit((unsigned char)*p))
                p++;
        }
        if (*p == '.') {
            p++;
            if (!isdigit((unsigned char)*p))
                return -1;
            while (isdigit((unsigned char)*p))
                p++;
        }
        if (*p == 'e' || *p == 'E') {
            p++;
            if (*p == '+' || *p == '-')
                p++;
            if (!isdigit((unsigned char)*p))
                return -1;
            while (isdigit((unsigned char)*p))
                p++;
        }
        char *end;
        errno = 0;
        double value = strtod(start, &end);
        if (end != p || errno == ERANGE || !isfinite(value) || value < 0)
            return -1;
        if (bit == 1) {
            if (value >= max_cards || value != (int)value)
                return -1;
            *phy_id = (int)value;
        } else
            *ratio = value;
        ratio_skip_space(&p);
        if (*p++ != (field == 0 ? ',' : '}'))
            return -1;
    }
    ratio_skip_space(&p);
    return seen == 3 && *p == '\0' ? 0 : -1;
}
#endif
