/*  YUView - YUV player with advanced analytics toolset
*   Copyright (C) 2015  Institut f�r Nachrichtentechnik
*                       RWTH Aachen University, GERMANY
*
*   YUView is free software; you can redistribute it and/or modify
*   it under the terms of the GNU General Public License as published by
*   the Free Software Foundation; either version 2 of the License, or
*   (at your option) any later version.
*
*   YUView is distributed in the hope that it will be useful,
*   but WITHOUT ANY WARRANTY; without even the implied warranty of
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*   GNU General Public License for more details.
*
*   You should have received a copy of the GNU General Public License
*   along with YUView.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DE265FILE_BITSTREAMHANDLER_H
#define DE265FILE_BITSTREAMHANDLER_H

#include <QFile>
#include <QMap>

#define BUFFER_SIZE 40960

// All the different NAL unit types (T-REC-H.265-201504 Page 85)
enum nal_unit_type
{
  TRAIL_N,        TRAIL_R,     TSA_N,          TSA_R,          STSA_N,      STSA_R,      RADL_N,     RADL_R,     RASL_N,     RASL_R, 
  RSV_VCL_N10,    RSV_VCL_N12, RSV_VCL_N14,    RSV_VCL_R11,    RSV_VCL_R13, RSV_VCL_R15, BLA_W_LP,   BLA_W_RADL, BLA_N_LP,   IDR_W_RADL,
  IDR_N_LP,       CRA_NUT,     RSV_IRAP_VCL22, RSV_IRAP_VCL23, RSV_VCL24,   RSV_VCL25,   RSV_VCL26,  RSV_VCL27,  RSV_VCL28,  RSV_VCL29,
  RSV_VCL30,      RSV_VCL31,   VPS_NUT,        SPS_NUT,        PPS_NUT,     AUD_NUT,     EOS_NUT,    EOB_NUT,    FD_NUT,     PREFIX_SEI_NUT,
  SUFFIX_SEI_NUT, RSV_NVCL41,  RSV_NVCL42,     RSV_NVCL43,     RSV_NVCL44,  RSV_NVCL45,  RSV_NVCL46, RSV_NVCL47, UNSPECIFIED
};

class de265File_FileHandler;

class nal_unit
{
protected:
  nal_unit(quint64 pos, nal_unit_type type, int layer, int temporalID) {
    filePos = pos;
    nal_type = type;
    nuh_layer_id = layer;
    nuh_temporal_id_plus1 = temporalID;
  }

  bool isIRAP() { return (nal_type == BLA_W_LP       || nal_type == BLA_W_RADL ||
                          nal_type == BLA_N_LP       || nal_type == IDR_W_RADL ||
                          nal_type == IDR_N_LP       || nal_type == CRA_NUT    ||
                          nal_type == RSV_IRAP_VCL22 || nal_type == RSV_IRAP_VCL23); }

  bool isSLNR() { return (nal_type == TRAIL_N     || nal_type == TSA_N       ||
                          nal_type == STSA_N      || nal_type == RADL_N      ||
                          nal_type == RASL_N      || nal_type == RSV_VCL_N10 ||
                          nal_type == RSV_VCL_N12 || nal_type == RSV_VCL_N14); }

  bool isRADL() { return (nal_type == RADL_N || nal_type == RADL_R); }
  bool isRASL() { return (nal_type == RASL_N || nal_type == RASL_R); }

  // Pointer to the first byte of the start code of the NAL unit
  quint64 filePos;

  // The information of the NAL unit header
  nal_unit_type nal_type;
  int nuh_layer_id;
  int nuh_temporal_id_plus1;
};

class vps : public nal_unit
{
public:
  vps(quint64 filePos, nal_unit_type type, int layer, int temporalID) :
    nal_unit(filePos, type, layer, temporalID) {};

  bool parse_vps(de265File_FileHandler *file);

  int vps_video_parameter_set_id; /// vps ID
  int vps_max_layers_minus1;		/// How many layers are there. Is this a scalable bitstream?
};

class sps : public nal_unit
{
public:
  sps(quint64 filePos, nal_unit_type type, int layer, int temporalID) :
    nal_unit(filePos, type, layer, temporalID) {};
  bool parse_sps(de265File_FileHandler *file);

  int sps_max_sub_layers_minus1;
  int sps_video_parameter_set_id;
  int sps_seq_parameter_set_id;
  int chroma_format_idc;
  int separate_colour_plane_flag;
  int pic_width_in_luma_samples;
  int pic_height_in_luma_samples;
  int conformance_window_flag;
  
  int conf_win_left_offset;
  int conf_win_right_offset;
  int conf_win_top_offset;
  int conf_win_bottom_offset;
  
  int bit_depth_luma_minus8;
  int bit_depth_chroma_minus8;
  int log2_max_pic_order_cnt_lsb_minus4;
};

class pps : public nal_unit
{
public:
  pps(quint64 filePos, nal_unit_type type, int layer, int temporalID) :
    nal_unit(filePos, type, layer, temporalID) {};
  bool parse_pps(de265File_FileHandler *file);
    
  int pps_pic_parameter_set_id;
  int pps_seq_parameter_set_id;

  bool output_flag_present_flag;
  int num_extra_slice_header_bits;
};

class slice : public nal_unit
{
public:
  slice(quint64 filePos, nal_unit_type type, int layer, int temporalID) :
    nal_unit(filePos, type, layer, temporalID),
    PicOrderCntVal{-1},
    PicOrderCntMsb{-1}
  {};

  bool parse_slice(de265File_FileHandler *file,
                   QMap<int, sps*> p_active_SPS_list,
                   QMap<int, pps*> p_active_PPS_list );

  bool first_slice_segment_in_pic_flag;
  int slice_pic_parameter_set_id;
  int slice_type;
  bool pic_output_flag;
  int colour_plane_id;
  int slice_pic_order_cnt_lsb;

  // Calculated values
  int PicOrderCntVal; // The slice POC
  int PicOrderCntMsb;

  // Static variables for keeping track of the decoding order
  static bool bFirstAUInDecodingOrder;
  static int prevTid0Pic_slice_pic_order_cnt_lsb;
  static int prevTid0Pic_PicOrderCntMsb;
};

class de265File_FileHandler
{
public:
  de265File_FileHandler(QString fileName);

  // Seek to the first byte of the payload data of the next NAL unit
  // Return false if not successfull (eg. file ended)
  bool seekToNextNALUnit();

  // Read the given number of bits and return as integer.
  // Returns -1 if an error occured while reading.
  int readBits(int nrBits);
  int readUE_V();

  // Move the file to the next byte boundary. Update the buffer if necessary.
  // Return false if the operation failed.
  bool gotoNextByte();

  // Get the current byte in the buffer
  char getCurByte() { return p_FileBuffer.at(p_posInBuffer); }

  // The current absolut position in the file (byte precise)
  quint64 tell() { return p_bufferStartPosInFile + p_posInBuffer; }

  // How many POC's have been found in the file
  int getNumberPOCs() { return p_POC_List.size(); }

protected:
  // The source binary file
  QFile *p_srcFile;
  QByteArray p_FileBuffer;
  quint64 p_FileBufferSize;
  int     p_posInBuffer;		    ///< The current position in the input buffer in bytes
  int     p_posInBuffer_bits;     ///< The current sub position in the input buffer in bits (0...7)
  quint64 p_bufferStartPosInFile; ///< The byte position in the file of the start of the currently loaded buffer
  int     p_numEmuPrevZeroBytes;  ///< The number of emulation prevention zero bytes that occured

  // The start code pattern
  QByteArray p_startCode;

  // A list of nal units sorted by position in the file.
  // Only parameter sets and random access positions go in here.
  // So basically all information we need to start the decoder at a certain position.
  QList<nal_unit*> p_nalUnitList;

  // A list of all POCs in the sequence (in coding order). POC's don't have to be consecutive, so the only
  // way to know how many pictures are in a sequences is to keep a list of all POCs.
  QList<int> p_POC_List;
  bool p_addPOCToList(int poc);   // Returns if the POC was already present int the list
  
  // Scan the file NAL by NAL. Keep track of all possible random access points and parameter sets in
  // p_nalUnitList. Also collect a list of all POCs in coding order in p_POC_List.
  bool scanFileForNalUnits();

  // load the next buffer
  bool updateBuffer();
};

#endif //DE265FILE_BITSTREAMHANDLER_H