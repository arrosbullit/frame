/*!
	@file		LittleBitstream2.cpp
	@brief
	@author		Robert Lluís, 2005
*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <assert.h>
#include "LittleBitstream2.h"

CLittle_Bitstream::CLittle_Bitstream()
{
   m_p_buffer                    = NULL;
   m_bytes_in_the_buffer         = 0;
   m_bit_to_read_in_current_byte = 0;
   m_bits_llegits                = 0;
   m_byte_to_read                = 0;
}

CLittle_Bitstream::CLittle_Bitstream(unsigned char *p_buffer, unsigned int buffer_size)
{
   m_p_buffer                    = p_buffer;
   m_bytes_in_the_buffer         = buffer_size;
   m_bit_to_read_in_current_byte = 0;
   m_bits_llegits                = 0;
   m_byte_to_read                = 0;
}

void CLittle_Bitstream::init(unsigned char *p_buffer, unsigned int buffer_size)
{
   m_p_buffer                    = p_buffer;
   m_bytes_in_the_buffer         = buffer_size;
   m_bit_to_read_in_current_byte = 0;
   m_bits_llegits                = 0;
   m_byte_to_read                = 0;

}

unsigned int CLittle_Bitstream::GetBits(unsigned int bits_to_get)
{
   if(bits_to_get == 0){
      return 0;
   }

   if(bits_to_get > 32)
   {
      throw("Error");
   }

   unsigned int bits_remaining_in_the_buffer = 0;

   bits_remaining_in_the_buffer = ((m_bytes_in_the_buffer - m_byte_to_read) << 3)
      -  m_bit_to_read_in_current_byte;

   if(bits_remaining_in_the_buffer < bits_to_get)
   {
      throw "Error";
   }

   m_bits_llegits += bits_to_get;

   int bit_index = m_bit_to_read_in_current_byte;
   int byte_index = m_byte_to_read;

   unsigned int return_value = 0;
   unsigned char aux_char = 0;
   unsigned int  aux_int = 0;

   if(bit_index == 0 && (bits_to_get & 7) == 0)
   {
      unsigned int bytes_to_get = bits_to_get >> 3;

      for(unsigned int i = 0; i < bytes_to_get; i++)
      {
         aux_char = m_p_buffer[byte_index++];

         aux_int = (unsigned int) (aux_char);

         return_value = (return_value << 8) | aux_int;

      }
   }
   else
   {
      for(unsigned int i = 0; i < bits_to_get; i++)
      {
         aux_char = m_p_buffer[byte_index] & (1 << (7 - bit_index));

         aux_int = (unsigned int) aux_char;

         aux_int = aux_int >> (7 - bit_index);

         return_value = (return_value << 1) | aux_int;

         bit_index++;
         if(bit_index >= 8)
         {
            bit_index = 0;
            byte_index++;
         }
      }
   }

   m_bit_to_read_in_current_byte =  bit_index;
   m_byte_to_read = byte_index;

   return return_value;
}

unsigned int CLittle_Bitstream::ShowBits(unsigned int bits_to_get)
{
   if(bits_to_get > 32)
   {
      throw("Error");
   }

   unsigned int bits_remaining_in_the_buffer = 0;

   bits_remaining_in_the_buffer = ((m_bytes_in_the_buffer - m_byte_to_read) << 3)
      -  m_bit_to_read_in_current_byte;

   if(bits_remaining_in_the_buffer < bits_to_get)
   {
      throw "Error";
   }

   int bit_index = m_bit_to_read_in_current_byte;
   int byte_index = m_byte_to_read;

   unsigned int return_value = 0;
   unsigned char aux_char = 0;
   unsigned int  aux_int = 0;

   if(bit_index == 0 && (bits_to_get & 7) == 0)
   {
      unsigned int bytes_to_get = bits_to_get >> 3;

      for(unsigned int i = 0; i < bytes_to_get; i++)
      {
         aux_char = m_p_buffer[byte_index++];

         aux_int = (unsigned int) (aux_char);

         return_value = (return_value << 8) | aux_int;

      }
   }
   else
   {
      for(unsigned int i = 0; i < bits_to_get; i++)
      {
         aux_char = m_p_buffer[byte_index] & (1 << (7 - bit_index));

         aux_int = (unsigned int) aux_char;

         aux_int = aux_int >> (7 - bit_index);

         return_value = (return_value << 1) | aux_int;


         bit_index++;
         if(bit_index >= 8)
         {
            bit_index = 0;
            byte_index++;
         }

      }
   }

   return return_value;
}

unsigned int CLittle_Bitstream::numero_de_bits_llegits()
{
   return m_bits_llegits;
}

void CLittle_Bitstream::go_back_n_bytes(unsigned int n_bytes)
{
   m_byte_to_read -= n_bytes;

   if(m_byte_to_read < 0)
   {
      m_byte_to_read = 0;
   }
}

unsigned CLittle_Bitstream::n_bits_in_buffer()
{
   unsigned int bits_remaining_in_the_buffer = 0;

   bits_remaining_in_the_buffer = ((m_bytes_in_the_buffer - m_byte_to_read) << 3)
      -  m_bit_to_read_in_current_byte;

   return bits_remaining_in_the_buffer;
}

void CLittle_Bitstream::get_byte_aligned()
{
   if(m_bit_to_read_in_current_byte != 0)
   {
      GetBits( 8 - m_bit_to_read_in_current_byte);
   }
}

