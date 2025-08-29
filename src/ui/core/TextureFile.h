#ifndef TEXTUREFILE_H
#define TEXTUREFILE_H


/**************


**************/


#include "../../core/MxUtils.h"
#include "../../io/Log.hpp"


namespace missionx
{


// --- Minimal header-only SHA256 implementation ---
//class SHA256 {
//public:
//  typedef uint8_t  uint8;
//  typedef uint32_t uint32;
//  typedef uint64_t uint64;
//
//  constexpr static size_t DIGEST_SIZE = 32;
//
//  SHA256() { init(); }
//
//  void init() {
//    m_dataLen = 0;
//    m_bitLen = 0;
//    m_state[0] = 0x6a09e667;
//    m_state[1] = 0xbb67ae85;
//    m_state[2] = 0x3c6ef372;
//    m_state[3] = 0xa54ff53a;
//    m_state[4] = 0x510e527f;
//    m_state[5] = 0x9b05688c;
//    m_state[6] = 0x1f83d9ab;
//    m_state[7] = 0x5be0cd19;
//  }
//
//  void update(const uint8* data, size_t len) {
//    for (size_t i = 0; i < len; ++i) {
//      m_data[m_dataLen] = data[i];
//      m_dataLen++;
//      if (m_dataLen == 64) {
//        transform();
//        m_bitLen += 512;
//        m_dataLen = 0;
//      }
//    }
//  }
//
//  void final(uint8 hash[DIGEST_SIZE]) {
//    size_t i = m_dataLen;
//
//    // Pad whatever data is left in the buffer.
//    if (m_dataLen < 56) {
//      m_data[i++] = 0x80;
//      while (i < 56)
//        m_data[i++] = 0x00;
//    } else {
//      m_data[i++] = 0x80;
//      while (i < 64)
//        m_data[i++] = 0x00;
//      transform();
//      memset(m_data, 0, 56);
//    }
//
//    m_bitLen += m_dataLen * 8;
//    m_data[63] = m_bitLen;
//    m_data[62] = m_bitLen >> 8;
//    m_data[61] = m_bitLen >> 16;
//    m_data[60] = m_bitLen >> 24;
//    m_data[59] = m_bitLen >> 32;
//    m_data[58] = m_bitLen >> 40;
//    m_data[57] = m_bitLen >> 48;
//    m_data[56] = m_bitLen >> 56;
//    transform();
//
//    // Convert state to little endian
//    for (i = 0; i < 4; ++i) {
//      for (int j = 0; j < 8; ++j) {
//        hash[i + (j * 4)] = (m_state[j] >> (24 - i * 8)) & 0x000000ff;
//      }
//    }
//  }
//
//private:
//  uint8  m_data[64];
//  uint32 m_dataLen;
//  uint64 m_bitLen;
//  uint32 m_state[8];
//
//  static uint32 rotr(uint32 x, uint32 n) { return (x >> n) | (x << (32 - n)); }
//  static uint32 choose(uint32 e, uint32 f, uint32 g) { return (e & f) ^ (~e & g); }
//  static uint32 majority(uint32 a, uint32 b, uint32 c) { return (a & b) ^ (a & c) ^ (b & c); }
//  static uint32 sig0(uint32 x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
//  static uint32 sig1(uint32 x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }
//
//  void transform() {
//    static const uint32 k[64] = {
//      0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
//      0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
//      0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
//      0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
//      0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
//      0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
//      0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
//      0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
//      0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
//      0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
//      0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
//      0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
//      0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,
//      0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
//      0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,
//      0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
//    };
//
//    uint32 m[64];
//    for (int i = 0, j = 0; i < 16; ++i, j += 4)
//      m[i] = (m_data[j] << 24) | (m_data[j+1] << 16) | (m_data[j+2] << 8) | (m_data[j+3]);
//    for (int i = 16; i < 64; ++i)
//      m[i] = sig1(m[i - 2]) + m[i - 7] + sig0(m[i - 15]) + m[i - 16];
//
//    uint32 a = m_state[0];
//    uint32 b = m_state[1];
//    uint32 c = m_state[2];
//    uint32 d = m_state[3];
//    uint32 e = m_state[4];
//    uint32 f = m_state[5];
//    uint32 g = m_state[6];
//    uint32 h = m_state[7];
//
//    for (int i = 0; i < 64; ++i) {
//      const uint32 t1 = h + (rotr(e,6) ^ rotr(e,11) ^ rotr(e,25)) + choose(e,f,g) + k[i] + m[i];
//      const uint32 t2 = (rotr(a,2) ^ rotr(a,13) ^ rotr(a,22)) + majority(a,b,c);
//      h = g;
//      g = f;
//      f = e;
//      e = d + t1;
//      d = c;
//      c = b;
//      b = a;
//      a = t1 + t2;
//    }
//
//    m_state[0] += a;
//    m_state[1] += b;
//    m_state[2] += c;
//    m_state[3] += d;
//    m_state[4] += e;
//    m_state[5] += f;
//    m_state[6] += g;
//    m_state[7] += h;
//  }
//};





class mxTextureFile
{
private:
public:
  typedef struct _mxIMAGEDATA
  {
    unsigned char* pData;
    int            Width;
    int            Height;
    int            Padding;
    short          Channels;

    _mxIMAGEDATA() //-V730
    {
      init();
    }

    void init()
    {
      Width    = 0;
      Height   = 0;
      Padding  = 0;
      Channels = 0;
    }

    [[nodiscard]] int   getW_i () const { return Width; }
    [[nodiscard]] int   getH_i () const { return Height; }
    [[nodiscard]] float getW_f () const { return static_cast<float> (Width); }
    [[nodiscard]] float getH_f () const { return static_cast<float> (Height); }

  } IMAGEDATA;

  int           TextureId;
  XPLMTextureID gTexture;

  // v25.08.1
  size_t      texture_hash_simple;
  std::string texture_hash_sha256;

  std::string fileName;
  std::string filePath;

  IMAGEDATA sImageData;


  mxTextureFile()
  {
    TextureId = 0;
    gTexture  = 0;
    texture_hash_simple = 0;
    texture_hash_sha256.clear();
    fileName.clear();
    filePath.clear();

    init();
  }

  void init()
  {
    sImageData.init(); // init struct

    fileName.clear();
    filePath.clear();
    TextureId = 0;
    gTexture  = 0;
  }

  std::string getAbsoluteFileLocation() { return filePath + XPLMGetDirectorySeparator() + fileName; }

  void setTextureFile(std::string inFileName, std::string inFilePath)
  {
    this->fileName = inFileName;
    this->filePath = inFilePath;
  }

  int getWidth() { return sImageData.getW_i(); }
  int getHeight() { return sImageData.getH_i(); }


  void store_hash()
  {
    this->texture_hash_simple = this->getTextureHash ();
    //this->texture_hash_sha256 = this->getTextureSHA256 ();
  }

private:
  [[nodiscard]] std::size_t getTextureHash() const {
    if (!sImageData.pData || sImageData.Width <= 0 || sImageData.Height <= 0 || sImageData.Channels <= 0) {
      return 0; // invalid / empty data
    }

    std::hash<unsigned char> byte_hash;
    std::size_t h = 0;

    // Total number of bytes = Width * Height * Channels (+ optional padding)
    std::size_t dataSize = static_cast<std::size_t>(sImageData.Width)
                         * static_cast<std::size_t>(sImageData.Height)
                         * static_cast<std::size_t>(sImageData.Channels);

    for (std::size_t i = 0; i < dataSize; ++i) {
      h ^= byte_hash(sImageData.pData[i]) + 0x9e3779b9 + (h << 6) + (h >> 2); // boost::hash_combine trick
    }
    return h;
  }

  //[[nodiscard]] std::string getTextureSHA256() const {
  //  if (!sImageData.pData || sImageData.Width <= 0 || sImageData.Height <= 0 || sImageData.Channels <= 0)
  //    return "";

  //  size_t dataSize = static_cast<size_t>(sImageData.Width) *
  //  static_cast<size_t>(sImageData.Height) *
  //  static_cast<size_t>(sImageData.Channels);

  //  SHA256 sha;
  //  sha.update(sImageData.pData, dataSize);
  //  unsigned char hash[SHA256::DIGEST_SIZE];
  //  sha.final(hash);

  //  std::ostringstream oss;
  //  // for (size_t i = 0; i < SHA256::DIGEST_SIZE; i++) {
  //  //   oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int> (hash[i]);
  //  // }
  //  for (const unsigned char i : hash) {
  //    oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int> (i);
  //  }
  //  return oss.str();
  //}

};

}



#endif // TEXTUREFILE_H
