#include <fstream>
#include <stdio.h>

namespace level_editor {
    #define BYTE 8

    template <class First, class Second>
    struct Pair {
        First first;
        Second second;

        Pair(){}
        Pair(First first, Second second): first(first), second(second){}
    };

    template <class Object>
    struct Node {
        Object current;
        Node *next = nullptr;
        
        Node(){}
        Node(Object current): current(current){}
    };

    template<class Object>
    class Stack {
    public:
        Stack(){ size = 0; }

        void push(Object object){
            if(size == 0){
                start.current = object;
                size++;
                return;
            }

            Node<Object> *temp = &start;
            for(unsigned int i = 0; i < size - 1; i++){
                temp = temp->next;
            }

            temp->next = new Node<Object>(object);
            size++;
        }

        Object pop(){
            Node<Object> *temp = &start;
            if(size == 1){
                size = 0;
                return start.current;
            }
            for(unsigned int i = 0; i < size - 2; i++) {
                temp = temp->next;
            }
            Object ret = temp->next->current;
            delete temp->next;
            size--;

            return ret;
        }

        unsigned int getSize(){ return size; }

    private:
        Node<Object> start;
        unsigned int size;
    };

    bool isCRLF(char *p){ return (*p == char(0x0d) && *(p + 1) == '\n'); }

    bool appendtoui(unsigned char *f, unsigned short *fi, unsigned int *v, unsigned int *o, unsigned short *b){
        for(unsigned int i = *fi; 0 < *b - *o; ++*fi){
            if(*v >= (unsigned int)(1 << *b)){ printf("error: value '%u' is greater than base '%u'", *v, 1 << *b); std::exit(-1); }
            *f += (*v & (1 << *o)) << i;

            ++*o;
            if(*fi == (sizeof(unsigned char) * BYTE) - 1){ *fi = 0; return true; }
        }

        return false;
    }

    unsigned short *getClosestBit(unsigned int *b){
        for(unsigned short i = 0; i < sizeof(unsigned int) * BYTE; i++){ if((unsigned int)(1 << i) > (*b - 1)){ return new unsigned short(i); }}
        return new unsigned short(~0);
    }

    char *File(char *&p, unsigned int *&s){
        printf("%s\n",p);
        std::ifstream fo(p, std::ios::in | std::ios::binary | std::ios::ate); // open the file
        if(!fo){printf("error opening '%s'\n", p); s = new unsigned int (0); return new char[1]{'\0'}; } // error handling for if can't open file

        s = new unsigned int(fo.tellg()); // get size then allocate in memory
        char *fd = new char[*s + 1];

        fo.seekg(0, std::ios::beg); // load in file
        fo.read(fd, *s);
        fo.close();
        fd[*s] = '\0';

        return fd;
    }

    bool Write(char *&p, char *&o, unsigned int *&os, char *ext){
        printf("writing: %s\n", p);
        std::ofstream fo(p);
        if(!fo){printf("error opening '%s'\n", p); return false; }

        if(ext[0] != 0){ fo << char(1) << ext << char(0); }
        else { fo << char(0); }
        // fo << getClosestBit(os) << char(0);
        for(unsigned int i = 0; i < *os; i++){ fo << o[i]; }
        fo.close();

        printf("writing complete :)\n");
        return true;
    }

    unsigned short gethex(char *p){
        if(*p >= '0' && *p <= '9'){ return *p - '0'; }
        if(*p >= 'a' && *p <= 'f'){ return *p - 'a' + 10; }
        printf("Error, could not get hex from char '%c' char hex '%x' at location: %p", *p, *p, p);
        std::exit(-1);
    }

    unsigned int ptoui(char *p){  return ((0xff & *(p + 3)) << 24 | (0xff & *(p + 2)) << 16 | (0xff & *(p + 1)) << 8 |(0xff & *p)); } // pointer to unsigned int
    unsigned int ptopx(char *p){  return ((0xff & *(p + 2)) << 16 | (0xff & *(p + 1)) << 8  | (0xff & *p)); } // pointer to px
    unsigned char ptouc(char *p){ return ((0xff & *(p + 1)) << 8  | (0xff & *p)); } // pointer to unsigned char
    unsigned int cstorgbhex(char *p){ return (gethex(p) << 20 | gethex(p + 1) << 16 | gethex(p + 2) << 12 | gethex(p + 3) << 8 | gethex(p + 4) << 4 | gethex(p + 5)); }

    //            fileData          fileSize      pixelDataOffset             width            height
    bool checkBMP(char *&f, unsigned int *&s, unsigned int *&pxdo, unsigned int *&w, unsigned int *&h, unsigned char *&bpx){
        if(*s < 26 || f[0] != 0x42 || f[1] != 0x4d || ptoui(f + 2) != *s){
            printf("%u < 26 || %d != 0x42 || %d != 0x4d || %x != %x\n", *s, f[0], f[1], ptoui(f + 2), *s);
            return false;
        }

        pxdo = new unsigned int( ptoui(f + 10));
        w =    new unsigned int( ptoui(f + 18));
        h =    new unsigned int( ptoui(f + 22));
        bpx =  new unsigned char(ptoui(f + 28) / BYTE);

        printf("size: %u\n", *s);
        printf("pxOffset: %u\n", *pxdo);
        printf("width: %u\n", *w);
        printf("height: %u\n", *h);
        printf("bits per px: %u\n", *bpx);

        return true;
    }

    bool stripKey(char *&f, unsigned int *&s, unsigned int *&kv, unsigned int *&kvs, unsigned int *&ov, unsigned int *&ovs){
        Stack<unsigned int> *stack = new Stack<unsigned int>();
        Stack<Pair<unsigned int, unsigned int>> *startAndSize = new Stack<Pair<unsigned int, unsigned int>>(); // pair<starting index of var, length of var>


        if(6 < *s){ stack->push(0); }
        unsigned int *p = new unsigned int(7);
        bool *crlf = new bool(isCRLF(f + 7));
        bool *fvar = new bool(f[*crlf? 8 : 7] != '\n'); // found variable
        printf("fvar: %d\n", *fvar);
        for(unsigned int i = 1; i < *s; i++){
            if(f[i] == '\n' && i + 6 < *s){
                stack->push(i + 1);

                if(*fvar){ startAndSize->push(Pair<unsigned int, unsigned int>(*p, (i - 1) - *p)); }
                *p = i + 8;
                *fvar = f[*crlf? 8 : 7] != '\n';
                printf("fvar: %d\n", *fvar);
            }
        }
        if(*fvar){ startAndSize->push(Pair<unsigned int, unsigned int>(*p, *s - *p)); }
        delete crlf;
        delete fvar;

        if(stack->getSize() == 0){ delete stack; return false; }

        ovs = new unsigned int(startAndSize->getSize() * 2);
        printf("ovs-s: %u\n", *ovs);
        ov = new unsigned int[*ovs];
        for(unsigned int i = *ovs; i > 0; i-=2){
            Pair<unsigned int, unsigned int> temp = startAndSize->pop();
            ov[i - 1] = temp.first;
            ov[i - 2] = temp.second;
        }
        delete startAndSize;

        kvs = new unsigned int(stack->getSize());
        kv = new unsigned int[*kvs];
        for(unsigned int i = *kvs; i > 0; i--){ kv[i - 1] = stack->pop(); }
        delete stack;

        return true;
    }

    bool BMPToBinary(char *&fd, unsigned int *&fs, char *&kd, unsigned int *&ks, char *&of, unsigned int *&ofs, unsigned int *&ov, unsigned int *&ovs, unsigned int *&pxdo, unsigned int *&w, unsigned int *&h, unsigned char *&bpx, unsigned int *&kv, unsigned int *&kvs){
        if(!checkBMP(fd, fs, pxdo, w, h, bpx)){  printf("Error: please make sure to use a valid bmp file\n");  return false; }
        if(!stripKey(kd, ks, kv, kvs, ov, ovs)){ printf("Error: please make sure to use a valid vars file\n"); return false; }

        Stack<unsigned char *> *fstack = new Stack<unsigned char *>(); // final stack

        unsigned char *f = new unsigned char(0);
        unsigned short *fi = new unsigned short(0);
        unsigned int *o = new unsigned int(0);
        unsigned short *b = getClosestBit(kvs);

        for(unsigned int y = 0; y < *h; y++){
            for(unsigned int x = *w - 1; x + 1 > 0; x--){
                for(unsigned int j = 0; j < *kvs; j++){
                    if((((y * *w) + x) * *bpx) + *pxdo >= *fs){ break; }
                    if(ptopx(fd + (((y * *w) + x) * *bpx) + *pxdo) == cstorgbhex(kd + kv[j])){
                        while(*o < *b){ if(appendtoui(f, fi, &j, o, b)){ fstack->push(f); f = new unsigned char(0); }; }
                        *o = 0;
                        break;
                    }
                }
            }
        }

        fstack->push(f);
        delete fi;
        delete b;
        delete o;

        unsigned int *fo = new unsigned int(5); //this is for num of tiles
        unsigned int *ofi = new unsigned int(5);
        for(unsigned int i = 0; i < *ovs; i+=2){ *fo += ov[i] + 1; }
        ++*fo;

        ofs = new unsigned int(fstack->getSize() + *fo);
        of = new char[*ofs];

        of[0] = char(0xff & *kvs); // setting tiles
        of[1] = char((0xff0000 & *kvs) >> 8);
        of[2] = char((0xff00 & *kvs) >> 16);
        of[3] = char((0xff000000 & *kvs) >> 24);
        of[4] = char(0);

        printf("ovs: %u", *ovs);
        for(unsigned int i = 0; i < *ovs; i+=2){
            for(unsigned int j = 0; j < ov[i]; j++){
                of[*ofi] = kd[ov[i + 1] + j];
                ++*ofi;
            }
            of[*ofi] = char(0);
            ++*ofi;
        }
        of[*ofi] = char(0);
        ++*ofi;

        for(; *ofi < *ofs; ++*ofi){ of[*ofi] = *fstack->pop(); }

        delete fo;
        delete ofi;

        return true;
    }

    bool BMPToBinary(int &argc, char **&argv){
        unsigned int *fSize, *kSize, *pixelDataOffset, *width, *height;
        unsigned int *keyValues, *keyValueSize;
        unsigned int *oSize, *oVars, *oVarsSize;
        unsigned char *bytesPerPixel;
        char *fData = File(argv[1], fSize);
        char *kData = File(argv[2], kSize);
        char *oData;
        char *ext = (argc > 4)? argv[4] : new char[4]{0};

        if(BMPToBinary(fData, fSize, kData, kSize, oData, oSize, oVars, oVarsSize, pixelDataOffset, width, height, bytesPerPixel, keyValues, keyValueSize)){
            Write(argv[3], oData, oSize, ext);
        }

        delete fSize;
        delete kSize;
        delete oSize;
        delete oVarsSize;
        delete pixelDataOffset;
        delete width;
        delete height;
        delete bytesPerPixel;
        delete keyValueSize;
        delete [] fData;
        delete [] kData;
        delete [] oData;
        delete [] oVars;
        delete [] keyValues;
        delete [] ext;
        return true;
    }
}

int main(int argc, char **argv){
    if(argc < 4){ printf("too few arguments: le.exe your.bmp your.vars output.level_data [extension example: HG]"); return -1; }
    level_editor::BMPToBinary(argc, argv);

    return 0;
}