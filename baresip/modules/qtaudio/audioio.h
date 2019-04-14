#ifndef AUDIOIO_H
#define AUDIOIO_H

class AudioIO {
public:
	virtual int input(char *ptr, int len) { (void)ptr; (void)len; return 0; }
	virtual int output(char *ptr, int len) { (void)ptr; (void)len; return 0; }
};

#endif // AUDIOIO_H
