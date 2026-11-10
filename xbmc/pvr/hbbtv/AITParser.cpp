/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AITParser.h"

#include "utils/Crc32.h"

#include <algorithm>
#include <functional>

using namespace PVR;

namespace
{
constexpr uint8_t AIT_TABLE_ID = 0x74;
constexpr size_t AIT_MIN_SECTION_SIZE = 12; // header and both loop lengths
constexpr size_t AIT_CRC_SIZE = 4;
constexpr size_t AIT_APPLICATION_HEADER_SIZE = 9;

// Descriptor tags of ETSI TS 102 809, table 2
constexpr uint8_t DESC_APPLICATION = 0x00;
constexpr uint8_t DESC_APPLICATION_NAME = 0x01;
constexpr uint8_t DESC_TRANSPORT_PROTOCOL = 0x02;
constexpr uint8_t DESC_SIMPLE_APPLICATION_LOCATION = 0x15;
constexpr uint8_t DESC_APPLICATION_USAGE = 0x16;

constexpr uint16_t TRANSPORT_PROTOCOL_HTTP = 0x0003;

struct TransportProtocol
{
  uint8_t label{0};
  std::string urlBase;
};

//! Bounds checked big endian reader. Reads past the end yield zero and clear Ok().
class CReader
{
public:
  CReader(const uint8_t* data, size_t size) : m_data(data), m_size(size) {}

  bool Ok() const { return m_ok; }
  size_t Pos() const { return m_pos; }
  size_t Remaining() const { return m_size - m_pos; }

  uint8_t U8() { return Take(1) ? m_data[m_pos - 1] : 0; }
  uint16_t U16() { return (static_cast<uint16_t>(U8()) << 8) | U8(); }
  uint32_t U32() { return (static_cast<uint32_t>(U16()) << 16) | U16(); }

  void Skip(size_t count) { Take(count); }

  //! Sub reader over the next count bytes, clamped to the available data.
  CReader Sub(size_t count)
  {
    const size_t available = std::min(count, Remaining());
    CReader sub(m_data + m_pos, available);
    Take(available);
    return sub;
  }

  std::string Text(size_t count)
  {
    const size_t available = std::min(count, Remaining());
    const uint8_t* start = m_data + m_pos;
    Take(available);
    return DecodeDvbText(start, available);
  }

private:
  bool Take(size_t count)
  {
    if (count > Remaining())
    {
      m_ok = false;
      m_pos = m_size;
      return false;
    }
    m_pos += count;
    return true;
  }

  // Strips the character table selection bytes of ETSI EN 300 468 annex A and treats the
  // remainder as single byte text.
  static std::string DecodeDvbText(const uint8_t* data, size_t size)
  {
    size_t skip = 0;
    if (size > 0 && data[0] < 0x20)
      skip = (data[0] == 0x10) ? 3 : 1;
    if (skip > size)
      return {};
    return std::string(reinterpret_cast<const char*>(data) + skip, size - skip);
  }

  const uint8_t* m_data;
  size_t m_size;
  size_t m_pos{0};
  bool m_ok{true};
};

void ParseDescriptorLoop(CReader& reader,
                         size_t length,
                         const std::function<void(uint8_t tag, CReader& body)>& onDescriptor)
{
  CReader loop = reader.Sub(length);
  while (loop.Remaining() >= 2)
  {
    const uint8_t tag = loop.U8();
    const uint8_t size = loop.U8();
    CReader body = loop.Sub(size);
    onDescriptor(tag, body);
  }
}

void ParseTransportProtocol(CReader& body, std::vector<TransportProtocol>& protocols)
{
  const uint16_t protocolId = body.U16();
  const uint8_t label = body.U8();
  if (protocolId != TRANSPORT_PROTOCOL_HTTP)
    return;

  const uint8_t urlBaseLength = body.U8();
  TransportProtocol protocol;
  protocol.label = label;
  protocol.urlBase = body.Text(urlBaseLength);
  if (body.Ok())
    protocols.emplace_back(std::move(protocol));
}

void ParseApplicationDescriptor(CReader& body,
                                CPVRHbbTVApplication& app,
                                std::vector<uint8_t>& transportLabels)
{
  const uint8_t profilesLength = body.U8();
  body.Skip(profilesLength);

  const uint8_t flags = body.U8();
  app.serviceBound = (flags & 0x80) != 0;
  app.visibility = (flags >> 5) & 0x03;
  app.priority = body.U8();

  while (body.Ok() && body.Remaining() > 0)
    transportLabels.push_back(body.U8());
}

std::string SelectUrlBase(const std::vector<TransportProtocol>& protocols,
                          const std::vector<uint8_t>& labels)
{
  for (const uint8_t label : labels)
  {
    const auto it = std::find_if(protocols.begin(), protocols.end(),
                                 [label](const TransportProtocol& p) { return p.label == label; });
    if (it != protocols.end())
      return it->urlBase;
  }
  return protocols.empty() ? std::string() : protocols.front().urlBase;
}
} // unnamed namespace

bool CAITParser::Parse(const uint8_t* data, size_t size, CAITSection& section)
{
  if (!data || size < AIT_MIN_SECTION_SIZE || data[0] != AIT_TABLE_ID)
    return false;

  const size_t sectionLength = ((data[1] & 0x0F) << 8) | data[2];
  size_t payloadSize = 0;
  if (sectionLength + 3 == size)
  {
    // CRC-32/MPEG-2 over the complete section including the CRC yields zero.
    Crc32 crc;
    crc.Compute(reinterpret_cast<const char*>(data), size);
    if (static_cast<uint32_t>(crc) != 0)
      return false;
    payloadSize = size - AIT_CRC_SIZE;
  }
  else if (sectionLength + 3 == size + AIT_CRC_SIZE)
    payloadSize = size; // CRC already verified and stripped by the sender
  else
    return false;

  CReader reader(data, payloadSize);
  reader.Skip(3); // table_id, section_syntax_indicator, section_length
  section.applicationType = reader.U16();
  const uint8_t versionByte = reader.U8();
  section.version = (versionByte >> 1) & 0x1F;
  section.currentNext = (versionByte & 0x01) != 0;
  section.sectionNumber = reader.U8();
  section.lastSectionNumber = reader.U8();
  section.applications.clear();

  std::vector<TransportProtocol> commonProtocols;
  const size_t commonLength = reader.U16() & 0x0FFF;
  ParseDescriptorLoop(reader, commonLength,
                      [&commonProtocols](uint8_t tag, CReader& body)
                      {
                        if (tag == DESC_TRANSPORT_PROTOCOL)
                          ParseTransportProtocol(body, commonProtocols);
                      });

  const size_t applicationLoopLength = reader.U16() & 0x0FFF;
  CReader loop = reader.Sub(applicationLoopLength);
  while (loop.Ok() && loop.Remaining() >= AIT_APPLICATION_HEADER_SIZE)
  {
    CPVRHbbTVApplication app;
    app.organisationId = loop.U32();
    app.applicationId = loop.U16();
    app.controlCode = loop.U8();
    const size_t descriptorsLength = loop.U16() & 0x0FFF;

    std::vector<TransportProtocol> protocols;
    std::vector<uint8_t> transportLabels;
    std::string initialPath;
    ParseDescriptorLoop(loop, descriptorsLength,
                        [&](uint8_t tag, CReader& body)
                        {
                          switch (tag)
                          {
                            case DESC_APPLICATION:
                              ParseApplicationDescriptor(body, app, transportLabels);
                              break;
                            case DESC_APPLICATION_NAME:
                              if (app.name.empty())
                              {
                                body.Skip(3); // ISO 639 language code
                                const uint8_t nameLength = body.U8();
                                app.name = body.Text(nameLength);
                              }
                              break;
                            case DESC_TRANSPORT_PROTOCOL:
                              ParseTransportProtocol(body, protocols);
                              break;
                            case DESC_SIMPLE_APPLICATION_LOCATION:
                              initialPath = body.Text(body.Remaining());
                              break;
                            case DESC_APPLICATION_USAGE:
                              app.usageType = body.U8();
                              break;
                            default:
                              break;
                          }
                        });

    protocols.insert(protocols.end(), commonProtocols.begin(), commonProtocols.end());
    app.url = SelectUrlBase(protocols, transportLabels) + initialPath;
    section.applications.emplace_back(std::move(app));
  }

  return reader.Ok();
}
