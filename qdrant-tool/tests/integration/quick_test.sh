#!/bin/bash
#
# Qdrant Tool 快速测试脚本
# 验证基本功能是否正常工作
#

set -e

QDRANT_TOOL="${QDRANT_TOOL:-./build/qdrant-tool}"
TEST_COLLECTION="quick_test_$(date +%s)"

echo "========================================"
echo "      Qdrant Tool Quick Test"
echo "========================================"
echo ""

# 检查工具
if [[ ! -x "${QDRANT_TOOL}" ]]; then
    echo "Error: qdrant-tool not found at ${QDRANT_TOOL}"
    exit 1
fi

echo "✓ Tool found"

# 测试 1: 版本
echo -n "Testing version... "
${QDRANT_TOOL} --version >/dev/null 2>&1 && echo "OK" || { echo "FAIL"; exit 1; }

# 测试 2: 创建集合
echo -n "Testing collection create... "
${QDRANT_TOOL} collection create --name ${TEST_COLLECTION} >/dev/null 2>&1 && echo "OK" || { echo "FAIL"; exit 1; }

# 测试 3: 添加记忆
echo -n "Testing memory add... "
ADD_RESULT=$(${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content "Test memory" --type test 2>&1)
echo "${ADD_RESULT}" | grep -q '"success": true' && echo "OK" || { echo "FAIL"; exit 1; }

# 测试 4: 搜索记忆
echo -n "Testing memory search... "
sleep 1
SEARCH_RESULT=$(${QDRANT_TOOL} search --collection ${TEST_COLLECTION} --query "test" 2>&1)
echo "${SEARCH_RESULT}" | grep -q '"success": true' && echo "OK" || { echo "FAIL"; exit 1; }

# 测试 5: 列出集合
echo -n "Testing collection list... "
${QDRANT_TOOL} collection list >/dev/null 2>&1 && echo "OK" || { echo "FAIL"; exit 1; }

# 测试 6: 集合信息
echo -n "Testing collection info... "
${QDRANT_TOOL} collection info --name ${TEST_COLLECTION} >/dev/null 2>&1 && echo "OK" || { echo "FAIL"; exit 1; }

# 清理
echo -n "Cleaning up... "
${QDRANT_TOOL} collection delete --name ${TEST_COLLECTION} >/dev/null 2>&1 && echo "OK" || { echo "FAIL"; exit 1; }

echo ""
echo "========================================"
echo "      All tests passed! ✓"
echo "========================================"
