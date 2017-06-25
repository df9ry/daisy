/* Copyright 2017 Tania Hagn
 *
 * This file is part of Daisy.
 *
 *    Daisy is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    Daisy is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with Daisy.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string>
#include <sstream>
#include <iostream>
#include <cctype>
#include <climits>
#include <algorithm>
#include <map>
#include <functional>

#include "utility.h"
#include "daisy_exception.h"
#include "rfm22b_types.h"

using namespace std;
using namespace RFM22B_NS;

namespace DaisyUtils {

	static string _decode_string(const string& s) {
		ostringstream oss;
		bool esc = false;
		for (string::const_iterator iter = s.begin();
				iter != s.end(); ++iter)
		{
			if (esc) {
				switch (*iter) {
				case 'b':
					oss << '\b';
					break;
				case 't':
					oss << '\t';
					break;
				case 'n':
					oss << '\n';
					break;
				case 'r':
					oss << '\r';
					break;
				default:
					oss << *iter;
					break;
				} // end switch //
				esc = false;
			} else if (*iter == '\\') {
				esc = true;
			} else {
				oss << *iter;
			}
		} // end for //
		if (esc)
			throw daisy_exception("Invalid string", s);
		return oss.str();
	}

	string decode_string(const string& s) {
		if ((s.length() < 2) || (s.find('"') == s.npos))
			return s;
		if (s[0] != '"')
			return _decode_string(s);
		if (s[s.length()-1] != '"')
			throw daisy_exception("Invalid string", s);
		return _decode_string(s.substr(1, s.length()-1));
	}

	static unsigned long long _decode_ulong(const std::string& s) {
		unsigned long long v = 0;
		for (string::const_iterator iter = s.begin();
				iter != s.end(); ++iter)
		{
			if (isdigit(*iter))
				v = 10 * v + (*iter - '0');
			else if (*iter != '_')
				throw daisy_exception("Invalid number value", s);
		} // end for //
		return v;
	}

	uint32_t decode_uint32(const std::string& s) {
		unsigned long long l = _decode_ulong(s);
		if (l > UINT32_MAX)
			throw daisy_exception("Value too large", s);
		return (uint32_t) l;
	}

	uint16_t decode_uint16(const std::string& s) {
		unsigned long long l = _decode_ulong(s);
		if (l > UINT16_MAX)
			throw daisy_exception("Value too large", s);
		return (uint16_t) l;
	}

	uint8_t decode_uint8(const std::string& s) {
		unsigned long long l = _decode_ulong(s);
		if (l > UINT8_MAX)
			throw daisy_exception("Value too large", s);
		return (uint8_t) l;
	}

	std::vector<uint8_t> decode_call(const std::string& s) {
		unsigned long long ssid = 0;
		std::string _s = s;
		std::transform(_s.begin(), _s.end(), _s.begin(), ::toupper);
		size_t pos = _s.find_last_of('-');
		if (pos != string::npos) {
			ssid = _decode_ulong(_s.substr(pos+1));
			if (ssid > 255)
				throw daisy_exception("SSID too large (0..255)", s);
			_s = _s.substr(0, pos);
		}
		if (_s.length() > 6)
			throw daisy_exception("Callsign too long (max. 6)", s);
		std::vector<uint8_t> result(6);
		for (int i = 0; i < 6; ++i) {
			uint8_t x;
			if (i < _s.length()) {
				x = (uint8_t)_s[i];
				if ((x < 0x20) || (x >= 0x60))
					throw daisy_exception(
							"Callsign contains invalid characters ", s);
			} else {
				x = 0x20;
			}
			result[i] = ( x - 0x20 ) << 2;
		} // end for //
		result[2] |= (( ssid & 0x03 ) >> 0 );
		result[3] |= (( ssid & 0x0c ) >> 2 );
		result[4] |= (( ssid & 0x30 ) >> 4 );
		result[5] |= (( ssid & 0xc0 ) >> 6 );
		return result;
	}

	bool decode_bool(const std::string& s) {
		string _s{tolower(s)};
		if ((_s == "true") || (_s == "on") || (_s == "1"))
			return true;
		if ((_s == "false") || (_s == "off") || (_s == "0"))
			return false;
		throw daisy_exception("Invalid operand", s);
	}

	RFM22B_Modulation_Type decode_modtype(const std::string& s) {
		typedef map<string, RFM22B_Modulation_Type> map_t;
		const map_t s2t {
			{ "UNMODULATED_CARRIER", RFM22B_Modulation_Type::UNMODULATED_CARRIER },
			{ "OOK",                 RFM22B_Modulation_Type::OOK                 },
			{ "FSK",                 RFM22B_Modulation_Type::FSK                 },
			{ "GFSK",                RFM22B_Modulation_Type::GFSK                }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	RFM22B_Modulation_Data_Source decode_mds (const std::string& s) {
		typedef map<string, RFM22B_Modulation_Data_Source> map_t;
		const map_t s2t {
			{ "DIRECT_GPIO", RFM22B_Modulation_Data_Source::DIRECT_GPIO },
			{ "DIRECT_SDI",  RFM22B_Modulation_Data_Source::DIRECT_SDI  },
			{ "FIFO",        RFM22B_Modulation_Data_Source::FIFO        },
			{ "PN9",         RFM22B_Modulation_Data_Source::PN9         }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	RFM22B_Data_Clock_Configuration decode_dcc (const std::string& s) {
		typedef map<string, RFM22B_Data_Clock_Configuration> map_t;
		const map_t s2t {
			{ "NONE", RFM22B_Data_Clock_Configuration::NONE  },
			{ "GPIO", RFM22B_Data_Clock_Configuration::GPIO  },
			{ "SDO",  RFM22B_Data_Clock_Configuration::SDO   },
			{ "NIRQ", RFM22B_Data_Clock_Configuration::NIRQ  }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	RFM22B_GPIO_Function decode_gpiofunc (const std::string& s) {
		typedef map<string, RFM22B_GPIO_Function> map_t;
		const map_t s2t {
			{ "POWER_ON_RESET",                  RFM22B_GPIO_Function::POWER_ON_RESET                  },
			{ "WAKE_UP_TIMER_1",                 RFM22B_GPIO_Function::WAKE_UP_TIMER_1                 },
			{ "LOW_BATTERY_DETECT_1",            RFM22B_GPIO_Function::LOW_BATTERY_DETECT_1            },
			{ "DIRECT_DIGITAL_INPUT",            RFM22B_GPIO_Function::DIRECT_DIGITAL_INPUT            },
			{ "EXTERNAL_INTERRUPT_FALLING",      RFM22B_GPIO_Function::EXTERNAL_INTERRUPT_FALLING      },
			{ "EXTERNAL_INTERRUPT_RISING",       RFM22B_GPIO_Function::EXTERNAL_INTERRUPT_RISING       },
			{ "EXTERNAL_INTERRUPT_CHANGE",       RFM22B_GPIO_Function::EXTERNAL_INTERRUPT_CHANGE       },
			{ "ADC_ANALOGUE_INPUT",              RFM22B_GPIO_Function::ADC_ANALOGUE_INPUT              },
			{ "DIRECT_DIGITAL_OUTPUT",           RFM22B_GPIO_Function::DIRECT_DIGITAL_OUTPUT           },
			{ "REFERENCE_VOLTAGE",               RFM22B_GPIO_Function::REFERENCE_VOLTAGE               },
			{ "DATA_CLOCK_OUTPUT",               RFM22B_GPIO_Function::DATA_CLOCK_OUTPUT               },
			{ "DATA_INPUT",                      RFM22B_GPIO_Function::DATA_INPUT                      },
			{ "EXTERNAL_RETRANSMISSION_REQUEST", RFM22B_GPIO_Function::EXTERNAL_RETRANSMISSION_REQUEST },
			{ "TX_STATE",                        RFM22B_GPIO_Function::TX_STATE                        },
			{ "TX_FIFO_ALMOST_FULL",             RFM22B_GPIO_Function::TX_FIFO_ALMOST_FULL             },
			{ "RX_DATA",                         RFM22B_GPIO_Function::RX_DATA                         },
			{ "RX_STATE",                        RFM22B_GPIO_Function::RX_STATE                        },
			{ "RX_FIFO_ALMOST_FULL",             RFM22B_GPIO_Function::RX_FIFO_ALMOST_FULL             },
			{ "ANTENNA_1_SWITCH",                RFM22B_GPIO_Function::ANTENNA_1_SWITCH                },
			{ "ANTENNA_2_SWITCH",                RFM22B_GPIO_Function::ANTENNA_2_SWITCH                },
			{ "VALID_PREAMBLE_DETECTED",         RFM22B_GPIO_Function::VALID_PREAMBLE_DETECTED         },
			{ "INVALID_PREAMBLE_DETECTED",       RFM22B_GPIO_Function::INVALID_PREAMBLE_DETECTED       },
			{ "SYNC_WORD_DETECTED",              RFM22B_GPIO_Function::SYNC_WORD_DETECTED              },
			{ "CLEAR_CHANNEL_ASSESSMENT",        RFM22B_GPIO_Function::CLEAR_CHANNEL_ASSESSMENT        },
			{ "VDD",                             RFM22B_GPIO_Function::VDD                             },
			{ "GND",                             RFM22B_GPIO_Function::GND                             }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	RFM22B_Interrupt decode_interrupt(const std::string& s) {
		typedef map<string, RFM22B_Interrupt> map_t;
		const map_t s2t {
			{ "POWER_ON_RESET_INT",       RFM22B_Interrupt::POWER_ON_RESET_INT       },
			{ "CHIP_READY",               RFM22B_Interrupt::CHIP_READY               },
			{ "LOW_BATTERY_DETECT",       RFM22B_Interrupt::LOW_BATTERY_DETECT       },
			{ "WAKE_UP_TIMER",            RFM22B_Interrupt::WAKE_UP_TIMER            },
			{ "RSSI",                     RFM22B_Interrupt::RSSI                     },
			{ "INVALID_PREAMBLE",         RFM22B_Interrupt::INVALID_PREAMBLE         },
			{ "VALID_PREAMBLE",           RFM22B_Interrupt::VALID_PREAMBLE           },
			{ "SYNC_WORD",                RFM22B_Interrupt::SYNC_WORD                },
			{ "CRC_ERROR",                RFM22B_Interrupt::CRC_ERROR                },
			{ "VALID_PACKET_RECEIVED",    RFM22B_Interrupt::VALID_PACKET_RECEIVED    },
			{ "PACKET_SENT",              RFM22B_Interrupt::PACKET_SENT              },
			{ "EXTERNAL",                 RFM22B_Interrupt::EXTERNAL                 },
			{ "RX_FIFO_ALMOST_FULL_INT",  RFM22B_Interrupt::RX_FIFO_ALMOST_FULL_INT  },
			{ "TX_FIFO_ALMOST_EMPTY_INT", RFM22B_Interrupt::TX_FIFO_ALMOST_EMPTY_INT },
			{ "TX_FIFO_ALMOST_FULL_INT",  RFM22B_Interrupt::TX_FIFO_ALMOST_FULL_INT  },
			{ "FIFO_UNDERFLOW_OVERFLOW",  RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW  }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	static RFM22B_Operating_Mode decode_opmode(const std::string& s) {
		typedef map<string, RFM22B_Operating_Mode> map_t;
		const map_t s2t {
			{ "TX_FIFO_RESET",             RFM22B_Operating_Mode::TX_FIFO_RESET             },
			{ "RX_FIFO_RESET",             RFM22B_Operating_Mode::RX_FIFO_RESET             },
			{ "LOW_DUTY_CYCLE_MODE",       RFM22B_Operating_Mode::LOW_DUTY_CYCLE_MODE       },
			{ "AUTOMATIC_TRANSMISSION",    RFM22B_Operating_Mode::AUTOMATIC_TRANSMISSION    },
			{ "RX_MULTI_PACKET",           RFM22B_Operating_Mode::RX_MULTI_PACKET           },
			{ "READY_MODE",                RFM22B_Operating_Mode::READY_MODE                },
			{ "TUNE_MODE",                 RFM22B_Operating_Mode::TUNE_MODE                 },
			{ "RX_MODE",                   RFM22B_Operating_Mode::RX_MODE                   },
			{ "TX_MODE",                   RFM22B_Operating_Mode::TX_MODE	                },
			{ "CRYSTAL_OSCILLATOR_SELECT", RFM22B_Operating_Mode::CRYSTAL_OSCILLATOR_SELECT },
			{ "ENABLE_WAKE_UP_TIMER",      RFM22B_Operating_Mode::ENABLE_WAKE_UP_TIMER      },
			{ "ENABLE_LOW_BATTERY_DETECT", RFM22B_Operating_Mode::ENABLE_LOW_BATTERY_DETECT },
			{ "RESET",                     RFM22B_Operating_Mode::RESET                     }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	RFM22B_Operating_Mode decode_opmodes(const std::string& s) {
		uint16_t x = 0;
		vector<string> v = split(s, ',');
		for (vector<string>::const_iterator iter = v.begin();
				iter != v.end(); ++iter)
		{
			x |= (uint16_t)decode_opmode(*iter);
		}
		return (RFM22B_Operating_Mode) x;
	}

	RFM22B_CRC_Mode decode_crcmode(const std::string& s) {
		typedef map<string, RFM22B_CRC_Mode> map_t;
		const map_t s2t {
			{ "CRC_DISABLED",  RFM22B_CRC_Mode::CRC_DISABLED  },
			{ "CRC_DATA_ONLY", RFM22B_CRC_Mode::CRC_DATA_ONLY },
			{ "CRC_NORMAL",    RFM22B_CRC_Mode::CRC_NORMAL    }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	RFM22B_CRC_Polynomial decode_crcpoly(const std::string& s) {
		typedef map<string, RFM22B_CRC_Polynomial> map_t;
		const map_t s2t {
			{ "CCITT",    RFM22B_CRC_Polynomial::CCITT    },
			{ "CRC16",    RFM22B_CRC_Polynomial::CRC16    },
			{ "IEC16",    RFM22B_CRC_Polynomial::IEC16    },
			{ "BIACHEVA", RFM22B_CRC_Polynomial::BIACHEVA }
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	static RFM22B_Modulation_Mode decode_modmode(const std::string& s) {
		typedef map<string, RFM22B_Modulation_Mode> map_t;
		const map_t s2t {
			{ "WHITENING",            RFM22B_Modulation_Mode::WHITENING            },
			{ "MANCHESTER",           RFM22B_Modulation_Mode::MANCHESTER           },
			{ "MANCHESTER_INVERSION", RFM22B_Modulation_Mode::MANCHESTER_INVERSION },
			{ "MANCHESTER_POLARITY",  RFM22B_Modulation_Mode::MANCHESTER_POLARITY  },
		};
		map_t::const_iterator i = s2t.find(toupper(s));
		if (i == s2t.end())
			throw daisy_exception("Value not found", s);
		return i->second;
	}

	RFM22B_Modulation_Mode decode_modmodes(const std::string& s) {
		uint16_t x = 0;
		vector<string> v = split(s, ',');
		for (vector<string>::const_iterator iter = v.begin();
				iter != v.end(); ++iter)
		{
			x |= (uint16_t)decode_modmode(*iter);
		}
		return (RFM22B_Modulation_Mode) x;
	}

	string print(const std::string& s) {
		ostringstream oss;
		oss << '"';
		for (string::const_iterator iter = s.begin();
				iter != s.end(); ++iter)
		{
			switch (*iter) {
			case '"' :
				oss << "\\\"";
				break;
			case '\b' :
				oss << "\\b";
				break;
			case '\t' :
				oss << "\\t";
				break;
			case '\n' :
				oss << "\\n";
				break;
			case '\r' :
				oss << "\\r";
				break;
			default :
				oss << *iter;
				break;
			} // end switch //
			oss << '"';
		} // end for //
		return oss.str();
	}

	string print(uint32_t v) {
		string s{to_string(v)};
		if (v < 1000) {
			return s;
		}
		ostringstream oss;
		for (int i = 0; i < s.length(); ++i) {
			if ((i > 0) && (((s.length() - i) % 3) == 0))
				oss << '_';
			oss << s[i];
		} // end for //
		return oss.str();
	}

	string print(uint16_t v) {
		return print((uint32_t)v);
	}

	string print(uint8_t v) {
		return print((uint32_t)v);
	}

	string print(int32_t v) {
		return to_string(v);
	}

	string print(int16_t v) {
		return print((int32_t)v);
	}

	string print(int8_t v) {
		return print((int32_t)v);
	}

	static const char hex[17] = "0123456789abcdef";

	static string _print_hex(const std::vector<uint8_t>& v) {
		ostringstream oss;
		bool first = true;
		for (vector<uint8_t>::const_iterator iter = v.begin();
				iter != v.end(); ++iter)
		{
			if (first)
				first = false;
			else
				oss << ':';
			oss << hex[(*iter) / 16] << hex[(*iter) % 16];
		}
		return oss.str();
	}

	string print_call(const std::vector<uint8_t>& v) {
		ostringstream oss;
		if (v.size() == 6) {
			for (int i = 0; i < 6; ++i)
			{
				char ch = ((( v[i] & 0xfc ) >> 2 )+ 0x20 );
				if (ch != ' ')
					oss << ch;
			}
			int ssid = (( v[5] & 0x03 ) << 6 ) |
					   (( v[4] & 0x03 ) << 4 ) |
					   (( v[3] & 0x03 ) << 2 ) |
					   (( v[2] & 0x03 ) << 0 );
			oss << "-" << ssid;
			if (v[0] & 0x01)
				oss << " (broadcast)";
			oss << " ";
		}
		oss << "[" <<_print_hex(v) << "]";
		return oss.str();
	}

	string print(bool v) {
		return (v?"ON":"OFF");
	}

	string print(const uint8_t* pb, size_t cb ) {
		if (!pb)
			return "NULL";
		ostringstream oss;
		oss << "[";
		bool first = true;
		while (cb > 0)
		{
			if (first)
				first = false;
			else
				oss << ':';
			oss << hex[(*pb) / 16] << hex[(*pb) % 16];
			++pb; --cb;
		}
		oss << "]";
		return oss.str();
	}

	string print(RFM22B_Modulation_Type v) {
		typedef map<RFM22B_Modulation_Type, string> map_t;
		const map_t t2s {
			{ RFM22B_Modulation_Type::UNMODULATED_CARRIER, "UNMODULATED_CARRIER" },
			{ RFM22B_Modulation_Type::OOK                , "OOK"                 },
			{ RFM22B_Modulation_Type::FSK                , "FSK"                 },
			{ RFM22B_Modulation_Type::GFSK               , "GFSK"                }
		};
		map_t::const_iterator i = t2s.find(v);
		if (i == t2s.end())
			throw daisy_exception("RFM22B_Modulation_Type value not found");
		return i->second;
	}

	string print(RFM22B_Modulation_Data_Source v) {
		typedef map<RFM22B_Modulation_Data_Source, string> map_t;
		const map_t t2s {
			{ RFM22B_Modulation_Data_Source::DIRECT_GPIO, "DIRECT_GPIO" },
			{ RFM22B_Modulation_Data_Source::DIRECT_SDI , "DIRECT_SDI"  },
			{ RFM22B_Modulation_Data_Source::FIFO       , "FIFO"        },
			{ RFM22B_Modulation_Data_Source::PN9        , "PN9"         }
		};
		map_t::const_iterator i = t2s.find(v);
		if (i == t2s.end())
			throw daisy_exception("RFM22B_Modulation_Data_Source value not found");
		return i->second;
	}

	string print(RFM22B_Data_Clock_Configuration v) {
		typedef map<RFM22B_Data_Clock_Configuration, string> map_t;
		const map_t t2s {
			{ RFM22B_Data_Clock_Configuration::NONE, "NONE"  },
			{ RFM22B_Data_Clock_Configuration::GPIO, "GPIO"  },
			{ RFM22B_Data_Clock_Configuration::SDO , "SDO"   },
			{ RFM22B_Data_Clock_Configuration::NIRQ, "NIRQ"  }
		};
		map_t::const_iterator i = t2s.find(v);
		if (i == t2s.end())
			throw daisy_exception("RFM22B_Data_Clock_Configuration value not found");
		return i->second;
	}

	string print(RFM22B_GPIO_Function v) {
		typedef map<RFM22B_GPIO_Function, string> map_t;
		const map_t t2s {
			{ RFM22B_GPIO_Function::POWER_ON_RESET                 , "POWER_ON_RESET"                  },
			{ RFM22B_GPIO_Function::WAKE_UP_TIMER_1                , "WAKE_UP_TIMER_1"                 },
			{ RFM22B_GPIO_Function::LOW_BATTERY_DETECT_1           , "LOW_BATTERY_DETECT_1"            },
			{ RFM22B_GPIO_Function::DIRECT_DIGITAL_INPUT           , "DIRECT_DIGITAL_INPUT"            },
			{ RFM22B_GPIO_Function::EXTERNAL_INTERRUPT_FALLING     , "EXTERNAL_INTERRUPT_FALLING"      },
			{ RFM22B_GPIO_Function::EXTERNAL_INTERRUPT_RISING      , "EXTERNAL_INTERRUPT_RISING"       },
			{ RFM22B_GPIO_Function::EXTERNAL_INTERRUPT_CHANGE      , "EXTERNAL_INTERRUPT_CHANGE"       },
			{ RFM22B_GPIO_Function::ADC_ANALOGUE_INPUT             , "ADC_ANALOGUE_INPUT"              },
			{ RFM22B_GPIO_Function::DIRECT_DIGITAL_OUTPUT          , "DIRECT_DIGITAL_OUTPUT"           },
			{ RFM22B_GPIO_Function::REFERENCE_VOLTAGE              , "REFERENCE_VOLTAGE"               },
			{ RFM22B_GPIO_Function::DATA_CLOCK_OUTPUT              , "DATA_CLOCK_OUTPUT"               },
			{ RFM22B_GPIO_Function::DATA_INPUT                     , "DATA_INPUT"                      },
			{ RFM22B_GPIO_Function::EXTERNAL_RETRANSMISSION_REQUEST, "EXTERNAL_RETRANSMISSION_REQUEST" },
			{ RFM22B_GPIO_Function::TX_STATE                       , "TX_STATE"                        },
			{ RFM22B_GPIO_Function::TX_FIFO_ALMOST_FULL            , "TX_FIFO_ALMOST_FULL"             },
			{ RFM22B_GPIO_Function::RX_DATA                        , "RX_DATA"                         },
			{ RFM22B_GPIO_Function::RX_STATE                       , "RX_STATE"                        },
			{ RFM22B_GPIO_Function::RX_FIFO_ALMOST_FULL            , "RX_FIFO_ALMOST_FULL"             },
			{ RFM22B_GPIO_Function::ANTENNA_1_SWITCH               , "ANTENNA_1_SWITCH"                },
			{ RFM22B_GPIO_Function::ANTENNA_2_SWITCH               , "ANTENNA_2_SWITCH"                },
			{ RFM22B_GPIO_Function::VALID_PREAMBLE_DETECTED        , "VALID_PREAMBLE_DETECTED"         },
			{ RFM22B_GPIO_Function::INVALID_PREAMBLE_DETECTED      , "INVALID_PREAMBLE_DETECTED"       },
			{ RFM22B_GPIO_Function::SYNC_WORD_DETECTED             , "SYNC_WORD_DETECTED"              },
			{ RFM22B_GPIO_Function::CLEAR_CHANNEL_ASSESSMENT       , "CLEAR_CHANNEL_ASSESSMENT"        },
			{ RFM22B_GPIO_Function::VDD                            , "VDD"                             },
			{ RFM22B_GPIO_Function::GND                            , "GND"                             }
		};
		map_t::const_iterator i = t2s.find(v);
		if (i == t2s.end())
			throw daisy_exception("RFM22B_GPIO_Function value not found");
		return i->second;
	}

	string print(RFM22B_Interrupt v) {
		typedef map<RFM22B_Interrupt, string> map_t;
		const map_t t2s {
			{ RFM22B_Interrupt::POWER_ON_RESET_INT      , "POWER_ON_RESET_INT"       },
			{ RFM22B_Interrupt::CHIP_READY              , "CHIP_READY"               },
			{ RFM22B_Interrupt::LOW_BATTERY_DETECT      , "LOW_BATTERY_DETECT"       },
			{ RFM22B_Interrupt::WAKE_UP_TIMER           , "WAKE_UP_TIMER"            },
			{ RFM22B_Interrupt::RSSI                    , "RSSI"                     },
			{ RFM22B_Interrupt::INVALID_PREAMBLE        , "INVALID_PREAMBLE"         },
			{ RFM22B_Interrupt::VALID_PREAMBLE          , "VALID_PREAMBLE"           },
			{ RFM22B_Interrupt::SYNC_WORD               , "SYNC_WORD"                },
			{ RFM22B_Interrupt::CRC_ERROR               , "CRC_ERROR"                },
			{ RFM22B_Interrupt::VALID_PACKET_RECEIVED   , "VALID_PACKET_RECEIVED"    },
			{ RFM22B_Interrupt::PACKET_SENT             , "PACKET_SENT"              },
			{ RFM22B_Interrupt::EXTERNAL                , "EXTERNAL"                 },
			{ RFM22B_Interrupt::RX_FIFO_ALMOST_FULL_INT , "RX_FIFO_ALMOST_FULL_INT"  },
			{ RFM22B_Interrupt::TX_FIFO_ALMOST_EMPTY_INT, "TX_FIFO_ALMOST_EMPTY_INT" },
			{ RFM22B_Interrupt::TX_FIFO_ALMOST_FULL_INT , "TX_FIFO_ALMOST_FULL_INT"  },
			{ RFM22B_Interrupt::FIFO_UNDERFLOW_OVERFLOW , "FIFO_UNDERFLOW_OVERFLOW"  }
		};
		map_t::const_iterator i = t2s.find(v);
		if (i == t2s.end())
			throw daisy_exception("RFM22B_Interrupt value not found");
		return i->second;
	}

	string print(RFM22B_Operating_Mode v) {
		typedef map<RFM22B_Operating_Mode, string> map_t;
		const map_t t2s {
			{ RFM22B_Operating_Mode::TX_FIFO_RESET            , "TX_FIFO_RESET"             },
			{ RFM22B_Operating_Mode::RX_FIFO_RESET            , "RX_FIFO_RESET"             },
			{ RFM22B_Operating_Mode::LOW_DUTY_CYCLE_MODE      , "LOW_DUTY_CYCLE_MODE"       },
			{ RFM22B_Operating_Mode::AUTOMATIC_TRANSMISSION   , "AUTOMATIC_TRANSMISSION"    },
			{ RFM22B_Operating_Mode::RX_MULTI_PACKET          , "RX_MULTI_PACKET"           },
			{ RFM22B_Operating_Mode::READY_MODE               , "READY_MODE"                },
			{ RFM22B_Operating_Mode::TUNE_MODE                , "TUNE_MODE"                 },
			{ RFM22B_Operating_Mode::RX_MODE                  , "RX_MODE"                   },
			{ RFM22B_Operating_Mode::TX_MODE	              , "TX_MODE"                   },
			{ RFM22B_Operating_Mode::CRYSTAL_OSCILLATOR_SELECT, "CRYSTAL_OSCILLATOR_SELECT" },
			{ RFM22B_Operating_Mode::ENABLE_WAKE_UP_TIMER     , "ENABLE_WAKE_UP_TIMER"      },
			{ RFM22B_Operating_Mode::ENABLE_LOW_BATTERY_DETECT, "ENABLE_LOW_BATTERY_DETECT" },
			{ RFM22B_Operating_Mode::RESET                    , "RESET"                     }
		};
		uint16_t x = (uint16_t) v;
		ostringstream oss;
		bool first = true;
		for (map_t::const_iterator iter = t2s.begin();
				iter != t2s.end(); ++iter)
		{
			uint16_t b = (uint16_t) iter->first;
			if (x & b) {
				map_t::const_iterator i = t2s.find((RFM22B_Operating_Mode)b);
				if (i == t2s.end())
					throw daisy_exception("RFM22B_Operating_Mode value not found");
				if (first)
					first = false;
				else
					oss << ',';
				oss << i->second;
			}
		}
		string result = oss.str();
		return (result.length() > 0)?result:"\"\"";
	}

	string print_status(uint8_t v) {
		ostringstream oss;
		oss << "CPS(" << (v & 0x03) << ")";
		if (v & 0x10)
			oss << ",HEADERR";
		if (v & 0x20)
			oss << ",RXFFEM";
		if (v & 0x40)
			oss << ",FFUNFL";
		if (v & 0x80)
			oss << ",FFOVFL";
		return oss.str();
	}

	string print(RFM22B_CRC_Mode v) {
		typedef map<RFM22B_CRC_Mode, string> map_t;
		const map_t t2s {
			{ RFM22B_CRC_Mode::CRC_DISABLED , "CRC_DISABLED"  },
			{ RFM22B_CRC_Mode::CRC_DATA_ONLY, "CRC_DATA_ONLY" },
			{ RFM22B_CRC_Mode::CRC_NORMAL   , "CRC_NORMAL"    }
		};
		map_t::const_iterator i = t2s.find(v);
		if (i == t2s.end())
			throw daisy_exception("RFM22B_CRC_Mode value not found");
		return i->second;
	}

	string print(RFM22B_CRC_Polynomial v) {
		typedef map<RFM22B_CRC_Polynomial, string> map_t;
		const map_t t2s {
			{ RFM22B_CRC_Polynomial::CCITT   , "CCITT"    },
			{ RFM22B_CRC_Polynomial::CRC16   , "CRC16"    },
			{ RFM22B_CRC_Polynomial::IEC16   , "IEC16"    },
			{ RFM22B_CRC_Polynomial::BIACHEVA, "BIACHEVA" }
		};
		map_t::const_iterator i = t2s.find(v);
		if (i == t2s.end())
			throw daisy_exception("RFM22B_CRC_Polynomial value not found");
		return i->second;
	}

	string print(RFM22B_Modulation_Mode v) {
		typedef map<RFM22B_Modulation_Mode, string> map_t;
		const map_t t2s {
			{ RFM22B_Modulation_Mode::WHITENING             , "WHITENING"             },
			{ RFM22B_Modulation_Mode::MANCHESTER            , "MANCHESTER"            },
			{ RFM22B_Modulation_Mode::MANCHESTER_INVERSION  , "MANCHESTER_INVERSION"  },
			{ RFM22B_Modulation_Mode::MANCHESTER_POLARITY   , "MANCHESTER_POLARITY"   },
		};
		uint16_t x = (uint16_t) v;
		ostringstream oss;
		bool first = true;
		for (map_t::const_iterator iter = t2s.begin();
				iter != t2s.end(); ++iter)
		{
			uint16_t b = (uint16_t) iter->first;
			if (x & b) {
				map_t::const_iterator i = t2s.find((RFM22B_Modulation_Mode)b);
				if (i == t2s.end())
					throw daisy_exception("RFM22_Modulation_Mode value not found");
				if (first)
					first = false;
				else
					oss << ',';
				oss << i->second;
			}
		}
		string result = oss.str();
		return (result.length() > 0)?result:"\"\"";
	}

	void noarg(const std::string& s) {
		if (s.length() > 0)
			throw daisy_exception("Misplaced argument", s);
	}

	static const string help_bool =
			"One of.\n"
			"ON|OFF";
	static const string help_call =
			"Callsign, max 6 characters, optional followed by"
			" -<SSID> where <SSID> is a number from 0 to 255";
	static const string help_qrg =
			"Frequency in Hz, ranging from 240_000_000 to"
			" 960_000_000";
	static const string help_uint32 =
			"Integer number ranging from 0 to 4_294_967_295";
	static const string help_uint8 =
			"Integer number ranging from 0 to 255";
	static const string help_noarg =
			"No arguments";
	static const string help_modtype =
			"One of:\n"
			"UNMODULATED_CARRIER|OOK|FSK|GFSK";
	static const string help_mds =
			"One of:\n"
			"DIRECT_GPIO|DIRECT_SDI|FIFO|PN9";
	static const string help_dcc =
			"One of:\n"
			"NONE|GPIO|SDO|NIRQ";
	static const string help_gpiofunc =
			"One of:\n"
			"POWER_ON_RESET|WAKE_UP_TIMER_1|LOW_BATTERY_DETECT_1|"
			"DIRECT_DIGITAL_INPUT|EXTERNAL_INTERRUPT_FALLING|"
			"EXTERNAL_INTERRUPT_RISING|EXTERNAL_INTERRUPT_CHANGE|"
			"ADC_ANALOGUE_INPUT|DIRECT_DIGITAL_OUTPUT|REFERENCE_VOLTAGE|"
			"DATA_CLOCK_OUTPUT|DATA_INPUT|EXTERNAL_RETRANSMISSION_REQUEST|"
			"TX_STATE|TX_FIFO_ALMOST_FULL|RX_DATA|RX_STATE|RX_FIFO_ALMOST_FULL|"
			"ANTENNA_1_SWITCH|ANTENNA_2_SWITCH|VALID_PREAMBLE_DETECTED|"
			"INVALID_PREAMBLE_DETECTED|SYNC_WORD_DETECTED|"
			"CLEAR_CHANNEL_ASSESSMENT|VDD|GND";
	static const string help_interrupt =
			"One of:\n"
			"POWER_ON_RESET_INT|CHIP_READY|LOW_BATTERY_DETECT|WAKE_UP_TIMER|"
			"RSSI|INVALID_PREAMBLE|VALID_PREAMBLE|SYNC_WORD|CRC_ERROR|"
			"VALID_PACKET_RECEIVED|PACKET_SENT|EXTERNAL|"
			"RX_FIFO_ALMOST_FULL_INT|TX_FIFO_ALMOST_EMPTY_INT|"
			"TX_FIFO_ALMOST_FULL_INT|FIFO_UNDERFLOW_OVERFLOW";
	static const string help_opmodes =
			"list of (separate with comma):\n"
			"AUTOMATIC_TRANSMISSION|RX_MULTI_PACKET|READY_MODE|TUNE_MODE|"
			"RX_MODE|TX_MODE|CRYSTAL_OSCILLATOR_SELECT|ENABLE_WAKE_UP_TIMER|"
			"ENABLE_LOW_BATTERY_DETECT|RESET";
	static const string help_modmodes =
			"list of (separate with comma):\n"
			"WHITENING|MANCHESTER|MANCHESTER_INVERSION|MANCHESTER_POLARITY";
	static const string help_crcmode =
			"One of:\n"
			"CRC_DISABLED|CRC_DATA_ONLY|CRC_NORMAL";
	static const string help_crcpoly =
			"One of:\n"
			"CCITT|CRC16|IEC16|BAICHEVA";
	static const string help_help =
			"One of:\n"
			"verbose|call|qrg|tune|rssi|ip|debug|reset|channel|deviation|"
			"datarate|modtype|mds|dcc|txpower|gpio0func|gpio1func|gpio2func|"
			"inte|intd|ints|opmodes|rxe|txe|txhdr|crcmode|crcpoly|txamft|"
			"txamet|rxamft|rxlen|txlen|rxclr|txclr|help|shell|devicetype|"
			"deviceversion|devicestatus|send|modmodes|bandwidth|preamble|"
			"receive|narrow|medium|wide";

	string print_help (const std::string& cmd) {
		typedef map<string, string> map_t;
		const map_t c2h {
			{ "verbose",       help_bool       },
			{ "call",          help_call       },
			{ "qrg",           help_qrg        },
			{ "tune",          help_uint32     },
			{ "rssi",          help_uint8      },
			{ "ip",            help_uint8      },
			{ "debug",         help_call       },
			{ "reset",         help_noarg      },
			{ "channel",       help_uint8      },
			{ "deviation",     help_uint32     },
			{ "datarate",      help_uint32     },
			{ "modtype",       help_modtype    },
			{ "mds",           help_mds        },
			{ "dcc",           help_dcc        },
			{ "txpower",       help_uint8      },
			{ "gpio0func",     help_gpiofunc   },
			{ "gpio1func",     help_gpiofunc   },
			{ "gpio2func",     help_gpiofunc   },
			{ "inte",          help_interrupt  },
			{ "intd",          help_interrupt  },
			{ "ints",          help_interrupt  },
			{ "opmodes",       help_opmodes    },
			{ "rxe",           help_noarg      },
			{ "txe",           help_noarg      },
			{ "txhdr",         help_uint32     },
			{ "crcmode",       help_crcmode    },
			{ "crcpoly",       help_crcpoly    },
			{ "txamft",        help_uint8      },
			{ "txamet",        help_uint8      },
			{ "rxamft",        help_uint8      },
			{ "rxlen",         help_noarg      },
			{ "txlen",         help_uint8      },
			{ "rxclr",         help_noarg      },
			{ "txclr",         help_noarg      },
			{ "help",          help_help       },
			{ "shell",         help_noarg      },
			{ "devicetype",    help_noarg      },
			{ "deviceversion", help_noarg      },
			{ "devicestatus",  help_noarg      },
			{ "send",          help_uint32     },
			{ "modmodes",      help_modmodes   },
			{ "bandwidth",     help_uint32     },
			{ "preamble",      help_uint32     },
			{ "receive",       help_uint32     },
			{ "narrow",        help_noarg      },
			{ "medium",        help_noarg      },
			{ "wide",          help_noarg      },
		};
		map_t::const_iterator i = c2h.find(tolower(cmd));
		if (i == c2h.end())
			return "Command not found: \"" + cmd + "\"";
		return i->second;
	}

	string toupper(const string& s) {
		string _s{s};
		transform(_s.begin(), _s.end(), _s.begin(), ::toupper);
		return _s;
	}

	string tolower(const string& s) {
		string _s{s};
		transform(_s.begin(), _s.end(), _s.begin(), ::tolower);
		return _s;
	}

	template<typename Out>
	static void split(const string& s, char delim, Out result) {
		stringstream ss;
		ss.str(s);
		string item;
		while (getline(ss, item, delim)) {
			*(result++) = item;
		}
	}

	vector<string> split(const string& s, char delim) {
		vector<string> elems;
		split(s, delim, back_inserter(elems));
		return elems;
	}

	// trim from start
	std::string ltrim(const std::string &_s) {
		string s{_s};
	    s.erase(s.begin(), std::find_if(s.begin(), s.end(),
	            std::not1(std::ptr_fun<int, int>(std::isspace))));
	    return s;
	}

	// trim from end
	std::string rtrim(const std::string &_s) {
		string s{_s};
	    s.erase(std::find_if(s.rbegin(), s.rend(),
	            std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
	    return s;
	}

	// trim from both ends
	std::string trim(const std::string &s) {
	    return ltrim(rtrim(s));
	}

} // end namespace //
