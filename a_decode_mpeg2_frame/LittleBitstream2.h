/*!
	@file		LittleBitstream2.h
	@brief
	@author		Robert Lluís, 2005
*/

#ifndef __LITTLE_BITSTREAM_2_H__
#define __LITTLE_BITSTREAM_2_H__

class CLittle_Bitstream
{
public:
	virtual ~CLittle_Bitstream(){}

	CLittle_Bitstream();

	CLittle_Bitstream(unsigned char *p_buffer, unsigned int buffer_size);

	void  init(unsigned char *p_buffer, unsigned int buffer_size);

	virtual unsigned int GetBits(unsigned int i);

	virtual unsigned int ShowBits(unsigned int i);

	unsigned int numero_de_bits_llegits();

	void go_back_n_bytes(unsigned int n_bytes);

	unsigned n_bits_in_buffer();

	void get_byte_aligned();

private:
   unsigned char    *m_p_buffer;
   int               m_byte_to_read;
   unsigned int      m_bytes_in_the_buffer;
   unsigned int      m_bit_to_read_in_current_byte;
   unsigned int      m_bits_llegits;

};

#endif
