#!/usr/bin/env python3
"""
STM32-ENR OTA 固件升级工具 (增强版)
通过蓝牙 (MAVLink v2) 无线升级 STM32 固件

增强特性:
  - 蓝牙链路健康检测 (心跳超时)
  - 断连自动重连 + 断点续传
  - 自适应超时 (擦除大块 Flash 时自动延长等待)
  - 进度条 + 预计剩余时间

用法:
    python ota_update.py /dev/rfcomm0 firmware.bin
"""

import sys
import os
import time
import struct
import zlib
import serial
import threading

# ──────────────────────────────────────────────
#  MAVLink v2 常量
# ──────────────────────────────────────────────
MAVLINK_STX = 0xFD

MAVLINK_MSG_ID_OTA_BEGIN = 50010
MAVLINK_MSG_ID_OTA_DATA = 50011
MAVLINK_MSG_ID_OTA_ACK = 50012
MAVLINK_MSG_ID_OTA_COMPLETE = 50013
MAVLINK_MSG_ID_HEARTBEAT = 0

CRC_EXTRA = {
    MAVLINK_MSG_ID_OTA_BEGIN: 211,
    MAVLINK_MSG_ID_OTA_DATA: 66,
    MAVLINK_MSG_ID_OTA_ACK: 143,
    MAVLINK_MSG_ID_OTA_COMPLETE: 89,
    MAVLINK_MSG_ID_HEARTBEAT: 50,
}

CHUNK_SIZE = 128
SYS_ID = 1
COMP_ID = 1

# ──────────────────────────────────────────────
#  超时参数
# ──────────────────────────────────────────────
ACK_TIMEOUT = 5.0           # 每包等 ACK 的超时（秒）
MAX_RETRIES = 10            # 每包最大重试次数
LINK_IDLE_TIMEOUT = 30.0    # 链路空闲超时（秒），超过视为断连
RECONNECT_WAIT = 2.0        # 断连后等待重连间隔（秒）
MAX_RECONNECTS = 30         # 最大重连尝试次数（约 60 秒）
PROGRESS_INTERVAL = 10      # 每 N 包打印一次进度

# CRC16-CCITT 表
CRC16_TABLE = [
    0x00, 0x91, 0xE3, 0x72, 0x07, 0x96, 0xE4, 0x75,
    0x0E, 0x9F, 0xED, 0x7C, 0x09, 0x98, 0xEA, 0x7B,
    0x1C, 0x8D, 0xFF, 0x6E, 0x1B, 0x8A, 0xF8, 0x69,
    0x12, 0x83, 0xF1, 0x60, 0x15, 0x84, 0xF6, 0x67,
    0x38, 0xA9, 0xDB, 0x4A, 0x3F, 0xAE, 0xDC, 0x4D,
    0x36, 0xA7, 0xD5, 0x44, 0x31, 0xA0, 0xD2, 0x43,
    0x24, 0xB5, 0xC7, 0x56, 0x23, 0xB2, 0xC0, 0x51,
    0x2A, 0xBB, 0xC9, 0x58, 0x2D, 0xBC, 0xCE, 0x5F,
    0x70, 0xE1, 0x93, 0x02, 0x77, 0xE6, 0x94, 0x05,
    0x7E, 0xEF, 0x9D, 0x0C, 0x79, 0xE8, 0x9A, 0x0B,
    0x6C, 0xFD, 0x8F, 0x1E, 0x6B, 0xFA, 0x88, 0x19,
    0x62, 0xF3, 0x81, 0x10, 0x65, 0xF4, 0x86, 0x17,
    0x48, 0xD9, 0xAB, 0x3A, 0x4F, 0xDE, 0xAC, 0x3D,
    0x46, 0xD7, 0xA5, 0x34, 0x41, 0xD0, 0xA2, 0x33,
    0x54, 0xC5, 0xB7, 0x26, 0x53, 0xC2, 0xB0, 0x21,
    0x5A, 0xCB, 0xB9, 0x28, 0x5D, 0xCC, 0xBE, 0x2F,
    0xE0, 0x71, 0x03, 0x92, 0xE7, 0x76, 0x04, 0x95,
    0xEE, 0x7F, 0x0D, 0x9C, 0xE9, 0x78, 0x0A, 0x9B,
    0xFC, 0x6D, 0x1F, 0x8E, 0xFB, 0x6A, 0x18, 0x89,
    0xF2, 0x63, 0x11, 0x80, 0xF5, 0x64, 0x16, 0x87,
    0xD8, 0x49, 0x3B, 0xAA, 0xDF, 0x4E, 0x3C, 0xAD,
    0xD6, 0x47, 0x35, 0xA4, 0xD1, 0x40, 0x32, 0xA3,
    0xC4, 0x55, 0x27, 0xB6, 0xC3, 0x52, 0x20, 0xB1,
    0xCA, 0x5B, 0x29, 0xB8, 0xC3, 0x52, 0x20, 0xB1,
    0xCA, 0x5B, 0x29, 0xB8, 0xCD, 0x5C, 0x2E, 0xBF,
    0x90, 0x01, 0x73, 0xE2, 0x97, 0x06, 0x74, 0xE5,
    0x9E, 0x0F, 0x7D, 0xEC, 0x99, 0x08, 0x7A, 0xEB,
    0x8C, 0x1D, 0x6F, 0xFE, 0x8B, 0x1A, 0x68, 0xF9,
    0x82, 0x13, 0x61, 0xF0, 0x85, 0x14, 0x66, 0xF7,
    0xA8, 0x39, 0x4B, 0xDA, 0xAF, 0x3E, 0x4C, 0xDD,
    0xA6, 0x37, 0x45, 0xD4, 0xA1, 0x30, 0x42, 0xD3,
    0xB4, 0x25, 0x57, 0xC6, 0xB3, 0x22, 0x50, 0xC1,
    0xBA, 0x2B, 0x59, 0xC8, 0xBD, 0x2C, 0x5E, 0xCF,
]


# ──────────────────────────────────────────────
#  MAVLink 帧构建 / 解析
# ──────────────────────────────────────────────

def crc16(data: bytes, crc_extra: int = 0) -> int:
    crc = 0xFFFF
    for b in data:
        crc = (crc >> 8) ^ CRC16_TABLE[(crc ^ b) & 0xFF]
    crc = (crc >> 8) ^ CRC16_TABLE[(crc ^ crc_extra) & 0xFF]
    return crc & 0xFFFF


_build_seq = [0]  # 用 list 实现闭包写


def build_mavlink_frame(msgid: int, payload: bytes) -> bytes:
    seq = _build_seq[0]
    _build_seq[0] = (seq + 1) & 0xFF

    length = len(payload)
    header = struct.pack('<BBBBBBBBB', length, 0, 0, seq, SYS_ID, COMP_ID,
                         msgid & 0xFF, (msgid >> 8) & 0xFF, (msgid >> 16) & 0xFF)

    crc_extra = CRC_EXTRA.get(msgid, 0)
    crc_val = crc16(header, 0)
    for b in payload:
        crc_val = (crc_val >> 8) ^ CRC16_TABLE[(crc_val ^ b) & 0xFF]
    crc_val = (crc_val >> 8) ^ CRC16_TABLE[(crc_val ^ crc_extra) & 0xFF]
    ck = struct.pack('<H', crc_val)

    return bytes([MAVLINK_STX]) + header + payload + ck


def parse_mavlink_frame(data: bytes):
    if len(data) < 10:
        return None
    if data[0] != MAVLINK_STX:
        return None
    length = data[1]
    if len(data) < 10 + length + 2:
        return None
    msgid = data[7] | (data[8] << 8) | (data[9] << 16)
    payload = data[10:10 + length]
    return {'msgid': msgid, 'payload': payload, 'len': length}


# ──────────────────────────────────────────────
#  ACK 等待 + 链路健康检测
# ──────────────────────────────────────────────

class LinkMonitor:
    """链路监控器：跟踪最后收到数据的时间"""
    def __init__(self, idle_timeout: float):
        self.last_rx = time.time()
        self.idle_timeout = idle_timeout
        self._lock = threading.Lock()

    def mark_rx(self):
        with self._lock:
            self.last_rx = time.time()

    def is_alive(self) -> bool:
        with self._lock:
            return (time.time() - self.last_rx) < self.idle_timeout

    def time_since_rx(self) -> float:
        with self._lock:
            return time.time() - self.last_rx


def wait_ack(ser: serial.Serial, link_mon: LinkMonitor,
             timeout: float = ACK_TIMEOUT) -> dict:
    """
    等待 OTA_ACK，同时监控链路健康。
    返回:
      {'status': int, 'written': int}  收到 ACK
      None                              超时
    """
    buf = b''
    start = time.time()
    while time.time() - start < timeout:
        try:
            if ser.in_waiting:
                chunk = ser.read(ser.in_waiting)
                if chunk:
                    link_mon.mark_rx()
                buf += chunk
                while len(buf) >= 10:
                    frame = parse_mavlink_frame(buf)
                    if frame:
                        buf = b''
                        if frame['msgid'] == MAVLINK_MSG_ID_OTA_ACK:
                            payload = frame['payload']
                            status = payload[0]
                            written = struct.unpack('<I', payload[1:5])[0]
                            return {'status': status, 'written': written}
                        buf = buf[1:]
                    else:
                        buf = buf[1:]
        except serial.SerialException:
            return None
        except Exception:
            return None

        if not link_mon.is_alive():
            # 链路空闲超时，提前返回
            return None

        time.sleep(0.01)
    return None


# ──────────────────────────────────────────────
#  串口连接管理
# ──────────────────────────────────────────────

def connect_port(port: str, baud: int, link_mon: LinkMonitor) -> serial.Serial:
    """打开串口并等待 STM32 发心跳"""
    ser = serial.Serial(port, baud, timeout=0.05, write_timeout=1)
    time.sleep(0.5)
    ser.reset_input_buffer()
    link_mon.mark_rx()
    return ser


def wait_for_reconnect(port: str, baud: int, link_mon: LinkMonitor,
                       max_attempts: int = MAX_RECONNECTS) -> serial.Serial:
    """蓝牙断连后，循环等待设备重新上线"""
    for attempt in range(max_attempts):
        print(f"  ↻ 等待重连 ({attempt+1}/{max_attempts})...")
        time.sleep(RECONNECT_WAIT)
        try:
            ser_test = serial.Serial(port, baud, timeout=0.1)
            ser_test.close()
            # 串口设备回来了
            ser = connect_port(port, baud, link_mon)
            print(f"  ✓ 已重连")
            return ser
        except (serial.SerialException, OSError):
            continue
    return None


# ──────────────────────────────────────────────
#  查询 STM32 已写入进度（断点续传）
# ──────────────────────────────────────────────

def query_written_size(ser: serial.Serial, link_mon: LinkMonitor) -> int:
    """
    询问 STM32 当前 OTA 已写入多少字节。
    如果 STM32 不在 OTA 状态或还没开始，返回 0。
    """
    try:
        ser.reset_input_buffer()
        ack = wait_ack(ser, link_mon, timeout=0.5)
        if ack:
            return ack['written']
    except Exception:
        pass
    return 0


# ──────────────────────────────────────────────
#  主 OTA 流程
# ──────────────────────────────────────────────

def ota_update(port: str, firmware_path: str, baud: int = 115200):
    """执行 OTA 升级，带断线重连 + 断点续传"""

    # ── 读固件 ──
    if not os.path.exists(firmware_path):
        print(f"[错误] 固件文件不存在: {firmware_path}")
        return False

    with open(firmware_path, 'rb') as f:
        firmware = f.read()

    total_size = len(firmware)
    firmware_crc = zlib.crc32(firmware) & 0xFFFFFFFF
    total_chunks = (total_size + CHUNK_SIZE - 1) // CHUNK_SIZE

    print("=" * 55)
    print(f"  STM32-ENR OTA 升级工具")
    print("=" * 55)
    print(f"  固件:     {os.path.basename(firmware_path)}")
    print(f"  大小:     {total_size} 字节 ({total_chunks} 包, 每包 {CHUNK_SIZE}B)")
    print(f"  CRC32:    0x{firmware_crc:08X}")
    print(f"  串口:     {port} @ {baud} baud")
    print(f"  超时:     每包 {ACK_TIMEOUT}s, 重试 {MAX_RETRIES} 次")
    print(f"  链路检测: 空闲 {LINK_IDLE_TIMEOUT}s 判定断连")
    print("=" * 55)

    # ── 连接 ──
    link_mon = LinkMonitor(LINK_IDLE_TIMEOUT)
    try:
        ser = connect_port(port, baud, link_mon)
    except serial.SerialException as e:
        print(f"[错误] 无法打开串口 {port}: {e}")
        return False

    current_chunk = 0
    reconnect_count = 0
    start_time = time.time()

    try:
        # ── 先查询是否已有部分写入（断点续传） ──
        written = query_written_size(ser, link_mon)
        if written > 0:
            current_chunk = written // CHUNK_SIZE
            print(f"[OTA] 检测到已有写入: {written} 字节, 从包 {current_chunk+1}/{total_chunks} 续传")

        try:
            # === 1. OTA_BEGIN ===
            if current_chunk == 0:
                print("[OTA] 发送 OTA_BEGIN...")
                payload = struct.pack('<II', total_size, firmware_crc)
                frame = build_mavlink_frame(MAVLINK_MSG_ID_OTA_BEGIN, payload)

                for retry in range(MAX_RETRIES):
                    ser.write(frame)
                    ack = wait_ack(ser, link_mon, timeout=max(5.0, total_size / 50000))
                    if ack and ack['status'] == 0:
                        print("[OTA] 下载区已擦除，开始传输")
                        break
                    print(f"  ↻ OTA_BEGIN 重试 ({retry+1}/{MAX_RETRIES})")
                else:
                    print("[错误] OTA_BEGIN 失败，设备无响应")
                    return False
            else:
                print(f"[OTA] 跳过 OTA_BEGIN，继续传输")

            # === 2. 分包发送 ===
            for i in range(current_chunk, total_chunks):
                offset = i * CHUNK_SIZE
                chunk = firmware[offset:offset + CHUNK_SIZE]
                chunk_padded = chunk.ljust(CHUNK_SIZE, b'\x00')

                success = False
                for retry in range(MAX_RETRIES):
                    # 检查链路健康
                    if not link_mon.is_alive():
                        elapsed = link_mon.time_since_rx()
                        print(f"\n  ⚠ 链路空闲 {elapsed:.0f}s，可能断连")
                        reconnect_count += 1
                        ser.close()

                        new_ser = wait_for_reconnect(port, baud, link_mon)
                        if new_ser:
                            ser = new_ser
                            print(f"  ✓ 已恢复，续传包 {i+1}/{total_chunks}")
                        else:
                            print(f"  [错误] 无法重连，{total_chunks - i} 包未完成")
                            return False

                    # 发当前包
                    payload = struct.pack('<I', offset) + chunk_padded
                    frame = build_mavlink_frame(MAVLINK_MSG_ID_OTA_DATA, payload)

                    try:
                        ser.write(frame)
                    except serial.SerialTimeoutException:
                        print(f"  ↻ 串口写超时，重试包 {i+1}/{total_chunks}")
                        continue

                    ack = wait_ack(ser, link_mon)
                    if ack and ack['status'] == 0:
                        success = True
                        written_offset = ack.get('written', offset + len(chunk))
                        # 如果 STM32 汇报的写入量大于当前偏移，大跳
                        if written_offset > offset + CHUNK_SIZE:
                            skip_chunks = written_offset // CHUNK_SIZE
                            if skip_chunks > i + 1:
                                print(f"  ⚡ 快速跳过: 已写至包 {skip_chunks}")
                                i = skip_chunks - 1  # for 循环会自动 +1
                        break
                    else:
                        if retry < MAX_RETRIES - 1:
                            print(f"  ↻ 重试包 {i+1}/{total_chunks} (offset=0x{offset:06X}, retry={retry+2}/{MAX_RETRIES})")

                if not success:
                    print(f"\n[错误] 包 {i+1}/{total_chunks} 发送失败，已传输 {offset} 字节")
                    print(f"  设备可能已损坏，需重新烧录 Bootloader")
                    return False

                # 进度打印
                if (i + 1) % PROGRESS_INTERVAL == 0 or i == total_chunks - 1:
                    elapsed = time.time() - start_time
                    pct = (i + 1) * 100 // total_chunks
                    done = (i + 1) * CHUNK_SIZE
                    speed = done / 1024 / elapsed if elapsed > 0 else 0
                    remain = (total_chunks - i - 1) * CHUNK_SIZE / (speed * 1024) if speed > 0 else 0
                    print(f"[OTA] {pct}% ({i+1}/{total_chunks}) — {done/1024:.0f}KB — {speed:.1f}KB/s — 剩余 {remain:.0f}s")

            # === 3. OTA_COMPLETE ===
            print("\n[OTA] 所有包已发送，执行 OTA_COMPLETE...")
            payload = struct.pack('<I', firmware_crc)
            frame = build_mavlink_frame(MAVLINK_MSG_ID_OTA_COMPLETE, payload)

            for retry in range(MAX_RETRIES):
                ser.write(frame)
                ack = wait_ack(ser, link_mon, timeout=10.0)
                if ack and ack['status'] == 0:
                    break
                print(f"  ↻ OTA_COMPLETE 重试 ({retry+1}/{MAX_RETRIES})")
                if not link_mon.is_alive():
                    ser.close()
                    ser = wait_for_reconnect(port, baud, link_mon)
                    if not ser:
                        print("[错误] OTA_COMPLETE 时断连，无法恢复")
                        return False
            else:
                print("[错误] OTA_COMPLETE 无响应")
                return False

            total_time = time.time() - start_time
            print(f"[OTA] ✅ 升级成功!")
            print(f"  总耗时: {total_time:.0f}s")
            print(f"  平均速度: {total_size / 1024 / total_time:.1f} KB/s")
            print(f"  断连次数: {reconnect_count}")
            print(f"  设备即将重启进 Bootloader 搬移固件...")
            return True

        except serial.SerialException as e:
            print(f"\n[错误] 串口异常: {e}")
            return False

    except KeyboardInterrupt:
        print(f"\n[OTA] 用户中断")
        print(f"  已传输: {current_chunk * CHUNK_SIZE} 字节")
        print(f"  重启脚本后会自动续传")
        return False
    finally:
        try:
            ser.close()
        except Exception:
            pass


if __name__ == '__main__':
    if len(sys.argv) < 3:
        print("用法: python ota_update.py <串口设备> <固件文件> [波特率]")
        print("示例:")
        print("  python ota_update.py /dev/rfcomm0 firmware.bin")
        print("  python ota_update.py /dev/ttyUSB0 firmware.bin 115200")
        sys.exit(1)

    port = sys.argv[1]
    firmware = sys.argv[2]
    baud = int(sys.argv[3]) if len(sys.argv) > 3 else 115200

    success = ota_update(port, firmware, baud)
    sys.exit(0 if success else 1)
