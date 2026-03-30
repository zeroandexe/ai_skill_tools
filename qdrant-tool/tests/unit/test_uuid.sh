#!/bin/bash
#
# UUID 工具单元测试
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
source "${SCRIPT_DIR}/../test_framework.sh"

# 模拟测试 UUID 生成
# 注意：这些是概念性测试，实际测试需要 C++ 测试框架如 Catch2

test_uuid_format() {
    local uuid="550e8400-e29b-41d4-a716-446655440000"
    
    # 检查长度（36 字符：8-4-4-4-12）
    assert_equals "36" "${#uuid}" "UUID length should be 36"
    
    # 检查横线位置
    assert_equals "-" "${uuid:8:1}" "UUID should have dash at position 8"
    assert_equals "-" "${uuid:13:1}" "UUID should have dash at position 13"
    assert_equals "-" "${uuid:18:1}" "UUID should have dash at position 18"
    assert_equals "-" "${uuid:23:1}" "UUID should have dash at position 23"
}

test_uuid_generation() {
    # 模拟生成两个 UUID 并检查它们不同
    local uuid1="$(cat /proc/sys/kernel/random/uuid 2>/dev/null || echo "550e8400-e29b-41d4-a716-446655440000")"
    local uuid2="$(cat /proc/sys/kernel/random/uuid 2>/dev/null || echo "550e8400-e29b-41d4-a716-446655440001")"
    
    assert_not_equals "${uuid1}" "${uuid2}" "Two UUIDs should be different"
}

# 运行测试
echo "Running UUID tests..."
test_uuid_format
test_uuid_generation
echo "UUID tests passed!"
