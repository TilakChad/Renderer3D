#include <stdint.h>
#include <stdio.h>

#define MAXBITS 15
#define MAXLCODES 286
#define MAXDCODES 30
#define MAXCODES 316
#define FIXLCODES 288

struct state
{
    uint8_t *input;
    uint32_t curinput;
    uint32_t totinput;
    uint8_t *output;
    uint32_t curoutput;
    uint32_t totoutput;
    uint32_t bitbuf;
    uint32_t bitcount;
};

static int uncompressed(struct state *);
static int dynamic(struct state *);
static int fixed(struct state *);
static int bits(struct state *, unsigned int);

struct huffman
{
    short *count; // number of symbols of each length
    short *symbol;
};

static int codes(struct state *s, struct huffman *, struct huffman *);

// Used to create huffman table
static int construct(struct huffman *, short *, int);

// decode binary code using generated huffman table
static int decode(struct state *, struct huffman *);

int deflate(uint8_t *buffer, uint32_t inlen, uint8_t *out, uint32_t *outlen)
{
    struct state s;
    s.input        = buffer;
    s.totinput     = inlen;
    s.curinput     = 0;
    s.curoutput    = 0;
    s.output       = out;
    s.totoutput    = *outlen;
    s.bitbuf       = 0;
    s.bitcount     = 0;
    int error_code = 0;
    int last_block;
    int compressed_type;
    do
    {
        last_block      = bits(&s, 1);
        compressed_type = bits(&s, 2);

        if (compressed_type == 0)
            error_code = uncompressed(&s);
        else if (compressed_type == 1)
            error_code = fixed(&s); //
        else if (compressed_type == 2)
            error_code = dynamic(&s);
        else
        {
            error_code = -1;
            break;
        }

    } while (!last_block);

    if (error_code == 0)
        *outlen = s.curoutput;
    else
        *outlen = 0;
    return error_code;
}

int bits(struct state *s, unsigned int required)
{
    unsigned val;
    val = s->bitbuf;
    while (s->bitcount < required)
    {
        if (s->curinput >= s->totinput)
            return -1;
        val |= s->input[s->curinput++] << s->bitcount;
        s->bitcount += 8;
    }
    if (s->bitcount >= required)
    {
        s->bitbuf = val >> required;
    }
    else
    {
        return -2;
    }
    s->bitcount -= required;
    // printf("Curent bitcount is : %d.", s->bitcount);
    return val & ((1 << required) - 1);
}

int dynamic(struct state *s)
{
    int hlit, hdist, hclen;
    int runcode;
    hlit  = bits(s, 5) + 257;
    hdist = bits(s, 5) + 1;
    hclen = bits(s, 4) + 4;
    int            index;
    short          lengths[MAXCODES];
    short          lencnt[MAXBITS + 1], lensym[MAXLCODES];
    short          distcnt[MAXBITS + 1], distsym[MAXDCODES];

    struct huffman lencode, distcode;

    lencode.count                = lencnt;
    lencode.symbol               = lensym;
    distcode.count               = distcnt;
    distcode.symbol              = distsym;

    static const short order[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};

    if (hlit > MAXLCODES || hdist > MAXDCODES)
        return -3; // bad counting

    for (index = 0; index < hclen; index++)
    {
        lengths[order[index]] = bits(s, 3);
    }
    for (; index < 19; index++)
    {
        lengths[order[index]] = 0;
    }
    // Run length codes
    // build huffman table for code lengths codes (use lencode temporarily)
    int err = construct(&lencode, lengths, 19);
    if (err)
        return -5;

    index = 0;
    while (index < hlit + hdist)
    {
        int symbol;
        int len;
        symbol = decode(s, &lencode);
        if (symbol < 0)
            return symbol;
        if (symbol < 16)
            lengths[index++] = symbol;
        else
        {
            int len = 0;
            if (symbol == 16)
            {
                if (index == 0)
                {
                    printf("NO previous code length .. .error...");
                    return -11;
                }
                len    = lengths[index - 1];
                symbol = 3 + bits(s, 2);
            }
            else if (symbol == 17)
            {
                symbol = 3 + bits(s, 3);
            }
            else if (symbol == 18)
            {
                symbol = 11 + bits(s, 7);
            }
            if (index + symbol > hdist + hlit)
            {
                return -6;
            }
            while (symbol--)
                lengths[index++] = len;
        }
    }

    err = construct(&lencode, lengths, hlit);
    if (err && (err < 0 || hlit != lencode.count[0] + lencode.count[1]))
    {
        printf("\nError -7.");
        return -7;
    }

    err = construct(&distcode, lengths + hlit, hdist);
    if (err && (err < 0 || hdist != distcode.count[0] + distcode.count[1]))
    {
        printf("\nErrorr -8..");
        return -8;
    }

    return codes(s, &lencode, &distcode);
}

int construct(struct huffman *h, short *length, int n)
{
    // Counter each number of symbol
    int left;
    for (int i = 0; i <= MAXBITS; ++i)
        h->count[i] = 0;

    for (int i = 0; i < n; ++i)
        h->count[length[i]]++;

    // Something like integiry check with huffman coded data
    left = 1;
    for (int i = 1; i <= MAXBITS; ++i)
    {
        left <<= 1;
        left -= h->count[i];
        if (left < 0)
            return left;
    }

    // if (!left)
    //    printf("Integrity passed.\n");

    short offs[MAXBITS + 1];
    offs[1] = 0;

    for (int i = 1; i < MAXBITS; ++i)
        offs[i + 1] = offs[i] + h->count[i];

    for (int i = 0; i < n; ++i)
        h->symbol[i] = -1;

    for (int symbol = 0; symbol < n; ++symbol)
        if (length[symbol])
            h->symbol[offs[length[symbol]]++] = symbol;

    // This function keeps the length in cannonical (or deflate stated form.);
    return 0;
}

// Since we are going to read huffman code, we have to read in bit reversed method

int decode(struct state *s, struct huffman *h)
{
    int count;
    int index;
    int code;
    int first;
    count = index = code = first = 0;

    for (int len = 1; len <= MAXBITS; ++len)
    {
        code |= bits(s, 1);
        count = h->count[len];
        if (code - count < first)
            return h->symbol[index + (code - first)];
        first += count; // Count the total number of smaller code lengths
        index += count;
        first <<= 1;
        code <<= 1;
    }
    return -2;
}

int codes(struct state *s, struct huffman *lencode, struct huffman *distcode)
{
    const short lenadd[]    = {3,  4,  5,  6,  7,  8,  9,  10, 11,  13,  15,  17,  19,  23, 27,
                            31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    const short extrabit[]  = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};

    const short distadd[]   = {1,   2,   3,   4,   5,   7,    9,    13,   17,   25,   33,   49,   65,    97,    129,
                             193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};

    const short distextra[] = {0, 0, 0, 0, 1, 1, 2, 2,  3,  3,  4,  4,  5,  5,  6,
                               6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};

    int         symbol, len;
    int         dist;
    do
    {
        symbol = decode(s, lencode);
        if (symbol < 0)
            return symbol;
        if (symbol < 256)
        {
            if (s->output != NULL)
            {
                if (s->curoutput > s->totoutput)
                    return 1;
                s->output[s->curoutput] = symbol;
            }
            s->curoutput++;
        }
        else if (symbol > 256)
        {
            int extra = symbol - 257;
            if (extra >= 29)
                return -10;
            len = lenadd[extra] + bits(s, extrabit[extra]);
            if (len > 258)
            {
                printf("Error in back traversing length...");
                return -23;
            }
            // check distance
            symbol = decode(s, distcode);
            if (symbol < 0)
                return symbol;
            if (symbol > 29)
                return -22;
            int k;
            k    = bits(s, distextra[symbol]);
            dist = distadd[symbol] + k;

            if (s->output != NULL)
            {
                if (s->curinput + len > s->totoutput)
                    return 1;
                while (len--)
                {
                    s->output[s->curoutput] = s->output[s->curoutput - dist];
                    s->curoutput++;
                }
            }
            else
            {
                s->curoutput += len;
            }
        }
    } while (symbol != 256);

    // done with valid dynamic block
    return 0;
}

int uncompressed(struct state *s)
{
    s->bitcount = 0;
    s->bitbuf   = 0;
    /** |       |       |                       |
     *  |len    | nlen  |   uncompressed data   |
     *  |       |       |                       |
     **/
    if (s->curinput + 4 > s->totinput)
        return 2;

    int len = s->input[s->curinput++];
    len |= s->input[s->curinput++] << 8;

    int nlen = s->input[s->curinput++];
    nlen |= s->input[s->curinput++] << 8;

    if ((len & 0xFFFF) == (~nlen & 0xFFFF))
        printf("Uncompressed data verified.\n");
    else
    {
        printf("Unverified uncompressed data.\n");
    }
    while (len--)
    {
        s->output[s->curoutput++] = s->input[s->curinput++];
    }

    return 0;
}

int fixed(struct state *s)
{
    static int     first = 1;
    short          lengths[MAXCODES];
    struct huffman lencode, distcode;
    short          lencnt[MAXBITS + 1], lensym[FIXLCODES];
    short          distcnt[MAXBITS + 1], distsym[MAXDCODES];
    lencode.count   = lencnt;
    lencode.symbol  = lensym;
    distcode.count  = distcnt;
    distcode.symbol = distsym;
    if (1)
    {
        for (int i = 0; i <= 143; ++i)
            lengths[i] = 8;
        for (int i = 144; i <= 255; ++i)
            lengths[i] = 9;
        for (int i = 256; i <= 279; ++i)
            lengths[i] = 7;
        for (int i = 280; i <= 287; ++i)
            lengths[i] = 8;
        first = 0;
    }
    construct(&lencode, lengths, FIXLCODES);

    for (int i = 0; i < MAXDCODES; ++i)
        lengths[i] = 5;

    construct(&distcode, lengths, MAXDCODES);
    return codes(s, &lencode, &distcode);
}