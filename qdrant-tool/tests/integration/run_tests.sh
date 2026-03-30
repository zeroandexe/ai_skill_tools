#!/bin/bash
#
# Qdrant Tool 集成测试脚本
# 运行所有测试并生成报告
#

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 测试配置
QDRANT_TOOL="${QDRANT_TOOL:-./build/qdrant-tool}"
TEST_COLLECTION="test_collection_$(date +%s)"
TEST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
RESULTS_DIR="${TEST_DIR}/results"
TEMP_DIR="${TEST_DIR}/temp"

# 计数器
TOTAL_TESTS=0
PASSED_TESTS=0
FAILED_TESTS=0
SKIPPED_TESTS=0

# 初始化
init() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}   Qdrant Tool Integration Tests${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    
    # 创建目录
    mkdir -p "${RESULTS_DIR}"
    mkdir -p "${TEMP_DIR}"
    
    # 检查工具
    if [[ ! -x "${QDRANT_TOOL}" ]]; then
        echo -e "${RED}Error: qdrant-tool not found at ${QDRANT_TOOL}${NC}"
        echo "Set QDRANT_TOOL environment variable to the correct path"
        exit 1
    fi
    
    echo "Test Tool: ${QDRANT_TOOL}"
    echo "Test Collection: ${TEST_COLLECTION}"
    echo "Results Dir: ${RESULTS_DIR}"
    echo ""
}

# 清理
cleanup() {
    echo ""
    echo -e "${BLUE}Cleaning up...${NC}"
    
    # 删除测试集合（如果存在）
    ${QDRANT_TOOL} collection delete --name "${TEST_COLLECTION}" >/dev/null 2>&1 || true
    rm -rf "${TEMP_DIR}"
    
    echo -e "${GREEN}Cleanup complete${NC}"
}

# 运行单个测试
run_test() {
    local test_name="$1"
    local test_cmd="$2"
    local expected_result="$3"  # "success" or "error"
    local check_pattern="$4"    # optional grep pattern to check in output
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -n "  [${TOTAL_TESTS}] ${test_name}... "
    
    # 运行命令并捕获输出
    local output
    local exit_code
    
    if output=$(eval "${test_cmd}" 2>&1); then
        exit_code=0
    else
        exit_code=$?
    fi
    
    # 保存输出
    echo "${output}" > "${RESULTS_DIR}/test_${TOTAL_TESTS}.log"
    
    # 检查结果
    local success=false
    
    if [[ "${expected_result}" == "success" && ${exit_code} -eq 0 ]]; then
        # 检查是否包含 success: true
        if echo "${output}" | grep -q '"success": true'; then
            success=true
        fi
    elif [[ "${expected_result}" == "error" && ${exit_code} -ne 0 ]]; then
        # 检查是否包含 success: false
        if echo "${output}" | grep -q '"success": false'; then
            success=true
        fi
    fi
    
    # 如果指定了检查模式，进一步验证
    if [[ -n "${check_pattern}" && "${success}" == "true" ]]; then
        if ! echo "${output}" | grep -q "${check_pattern}"; then
            success=false
        fi
    fi
    
    # 输出结果
    if [[ "${success}" == "true" ]]; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
        return 0
    else
        echo -e "${RED}FAIL${NC}"
        echo "      Exit code: ${exit_code}"
        echo "      Expected: ${expected_result}"
        echo "      Output: ${output}" | head -3
        FAILED_TESTS=$((FAILED_TESTS + 1))
        return 1
    fi
}

# 测试套件 1: 版本和帮助
test_version_and_help() {
    echo -e "${YELLOW}Test Suite 1: Version and Help${NC}"
    
    # 版本测试（特殊处理，因为输出不是 JSON）
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "  [${TOTAL_TESTS}] Show version... "
    local version_output
    if version_output=$(${QDRANT_TOOL} --version 2>&1) && echo "${version_output}" | grep -q "1.0.0"; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    # 帮助测试（特殊处理）
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "  [${TOTAL_TESTS}] Show help... "
    local help_output
    if help_output=$(${QDRANT_TOOL} --help 2>&1) && echo "${help_output}" | grep -q "qdrant-tool"; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    # Add 帮助测试
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "  [${TOTAL_TESTS}] Show add help... "
    if help_output=$(${QDRANT_TOOL} add --help 2>&1) && echo "${help_output}" | grep -q "add"; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    # Search 帮助测试
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    echo -n "  [${TOTAL_TESTS}] Show search help... "
    if help_output=$(${QDRANT_TOOL} search --help 2>&1) && echo "${help_output}" | grep -q "search"; then
        echo -e "${GREEN}PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "${RED}FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    
    echo ""
}

# 测试套件 2: 集合管理
test_collection_management() {
    echo -e "${YELLOW}Test Suite 2: Collection Management${NC}"
    
    # 列出集合（可能为空或已有集合）
    run_test "List collections" "${QDRANT_TOOL} collection list" "success" "collections"
    
    # 创建测试集合
    run_test "Create collection" "${QDRANT_TOOL} collection create --name ${TEST_COLLECTION} --dimension 1024" "success" "success"
    
    # 获取集合信息
    run_test "Get collection info" "${QDRANT_TOOL} collection info --name ${TEST_COLLECTION}" "success" "dimension"
    
    # 尝试创建已存在的集合（应该失败）
    # run_test "Create existing collection (should fail)" "${QDRANT_TOOL} collection create --name ${TEST_COLLECTION}" "error"
    
    echo ""
}

# 测试套件 3: 记忆添加
test_memory_add() {
    echo -e "${YELLOW}Test Suite 3: Memory Add${NC}"
    
    # 基本添加
    run_test "Add simple memory" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content 'Test memory content'" "success" "id"
    
    # 带类型
    run_test "Add with type" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content 'User likes coffee' --type preference" "success" "id"
    
    # 带标签
    run_test "Add with tags" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content 'Important task' --type todo --tag important --tag work" "success" "id"
    
    # 带指定ID（必须是 UUID 格式）
    run_test "Add with specific ID" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --id '550e8400-e29b-41d4-a716-446655440000' --content 'Memory with custom ID'" "success" "550e8400-e29b-41d4-a716-446655440000"
    
    # 错误：缺少 content
    run_test "Add without content (should fail)" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION}" "error"
    
    echo ""
}

# 测试套件 4: 记忆搜索
test_memory_search() {
    echo -e "${YELLOW}Test Suite 4: Memory Search${NC}"
    
    # 等待一下确保数据已索引
    sleep 1
    
    # 基本搜索
    run_test "Search basic" "${QDRANT_TOOL} search --collection ${TEST_COLLECTION} --query 'coffee'" "success" "results"
    
    # 带 limit
    run_test "Search with limit" "${QDRANT_TOOL} search --collection ${TEST_COLLECTION} --query 'test' --limit 2" "success" "\"count\":"
    
    # 带 min-score
    run_test "Search with min-score" "${QDRANT_TOOL} search --collection ${TEST_COLLECTION} --query 'coffee' --min-score 0.5" "success" "results"
    
    # 带 filter-type
    run_test "Search with type filter" "${QDRANT_TOOL} search --collection ${TEST_COLLECTION} --query 'task' --filter-type todo" "success" "results"
    
    # 错误：缺少 query
    run_test "Search without query (should fail)" "${QDRANT_TOOL} search --collection ${TEST_COLLECTION}" "error"
    
    echo ""
}

# 测试套件 5: 记忆更新
test_memory_update() {
    echo -e "${YELLOW}Test Suite 5: Memory Update${NC}"
    
    # 先添加一个用于更新的记忆
    local add_output
    add_output=$(${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content 'Original content' --type fact 2>&1)
    local id=$(echo "${add_output}" | grep -o '"id": "[^"]*"' | cut -d'"' -f4)
    
    if [[ -z "${id}" ]]; then
        echo -e "${RED}  Failed to get ID for update test${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 3))
        TOTAL_TESTS=$((TOTAL_TESTS + 3))
        echo ""
        return
    fi
    
    echo "  Using test ID: ${id}"
    
    # 更新内容
    run_test "Update content" "${QDRANT_TOOL} update --collection ${TEST_COLLECTION} --id '${id}' --content 'Updated content'" "success" "success"
    
    # 更新类型
    run_test "Update type" "${QDRANT_TOOL} update --collection ${TEST_COLLECTION} --id '${id}' --type updated_type" "success" "success"
    
    # 错误：不存在的 ID
    run_test "Update non-existent ID" "${QDRANT_TOOL} update --collection ${TEST_COLLECTION} --id 'non_existent_id_999' --content 'test'" "error"
    
    echo ""
}

# 测试套件 6: 记忆删除
test_memory_delete() {
    echo -e "${YELLOW}Test Suite 6: Memory Delete${NC}"
    
    # 先添加一个用于删除的记忆
    local add_output
    add_output=$(${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content 'To be deleted' 2>&1)
    local id=$(echo "${add_output}" | grep -o '"id": "[^"]*"' | cut -d'"' -f4)
    
    if [[ -n "${id}" ]]; then
        echo "  Using test ID: ${id}"
        run_test "Delete memory" "${QDRANT_TOOL} delete --collection ${TEST_COLLECTION} --id '${id}'" "success" "success"
    else
        echo -e "${RED}  Failed to get ID for delete test${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
        TOTAL_TESTS=$((TOTAL_TESTS + 1))
    fi
    
    # 错误：删除不存在的 ID
    run_test "Delete non-existent ID" "${QDRANT_TOOL} delete --collection ${TEST_COLLECTION} --id 'non_existent_id_999'" "error"
    
    echo ""
}

# 测试套件 7: 文件导入
test_file_import() {
    echo -e "${YELLOW}Test Suite 7: File Import${NC}"
    
    # 创建测试文件
    local test_file="${TEMP_DIR}/test_document.txt"
    cat > "${test_file}" << 'EOF'
This is a test document for importing.
It contains multiple lines of text.
We want to test the chunking functionality.
This document will be split into chunks.
Each chunk will be stored separately.
EOF
    
    run_test "Import file with defaults" "${QDRANT_TOOL} import --file '${test_file}' --collection ${TEST_COLLECTION}_import" "success" "total_chunks"
    
    # 自定义分块大小
    run_test "Import with custom chunk size" "${QDRANT_TOOL} import --file '${test_file}' --collection ${TEST_COLLECTION}_import2 --chunk-size 50 --chunk-overlap 10" "success" "total_chunks"
    
    # 错误：不存在的文件
    run_test "Import non-existent file" "${QDRANT_TOOL} import --file '/non/existent/file.txt' --collection ${TEST_COLLECTION}" "error"
    
    echo ""
}

# 测试套件 8: 错误处理
test_error_handling() {
    echo -e "${YELLOW}Test Suite 8: Error Handling${NC}"
    
    # 无效命令
    run_test "Invalid command" "${QDRANT_TOOL} invalid_command" "error"
    
    # 搜索不存在的集合
    run_test "Search non-existent collection" "${QDRANT_TOOL} search --collection non_existent_collection_xyz --query 'test'" "error"
    
    # 缺少必需参数
    run_test "Collection create without name" "${QDRANT_TOOL} collection create" "error"
    
    echo ""
}

# 测试套件 9: 性能测试
test_performance() {
    echo -e "${YELLOW}Test Suite 9: Performance Test${NC}"
    
    echo "  Testing batch add performance..."
    
    local start_time=$(date +%s.%N)
    local count=5
    
    for i in $(seq 1 ${count}); do
        ${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content "Performance test memory ${i}" --type perf_test >/dev/null 2>&1
    done
    
    local end_time=$(date +%s.%N)
    local duration=$(echo "${end_time} - ${start_time}" | bc)
    local avg_time=$(echo "scale=2; ${duration} / ${count}" | bc)
    
    echo -e "  ${GREEN}Added ${count} memories in ${duration}s (avg: ${avg_time}s/op)${NC}"
    
    # 性能阈值检查（假设 5 秒/操作是可接受的）
    local max_acceptable=$(echo "${count} * 5" | bc)
    if (( $(echo "${duration} < ${max_acceptable}" | bc -l) )); then
        echo -e "  ${GREEN}Performance test: PASS${NC}"
        PASSED_TESTS=$((PASSED_TESTS + 1))
    else
        echo -e "  ${RED}Performance test: FAIL${NC}"
        FAILED_TESTS=$((FAILED_TESTS + 1))
    fi
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo ""
}

# 测试套件 10: 特殊字符处理
test_special_characters() {
    echo -e "${YELLOW}Test Suite 10: Special Characters${NC}"
    
    # 中文内容
    run_test "Chinese content" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content '用户喜欢喝冰美式咖啡' --type preference" "success" "id"
    
    # 特殊符号
    run_test "Special symbols" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content 'Test with special chars: !@#$%^&*()_+{}|:<>?~'" "success" "id"
    
    # 换行内容
    run_test "Multiline content" "${QDRANT_TOOL} add --collection ${TEST_COLLECTION} --content $'Line 1\nLine 2\nLine 3'" "success" "id"
    
    echo ""
}

# 生成测试报告
generate_report() {
    echo -e "${BLUE}========================================${NC}"
    echo -e "${BLUE}           Test Report${NC}"
    echo -e "${BLUE}========================================${NC}"
    echo ""
    echo "Total Tests:  ${TOTAL_TESTS}"
    echo -e "${GREEN}Passed:       ${PASSED_TESTS}${NC}"
    echo -e "${RED}Failed:       ${FAILED_TESTS}${NC}"
    
    if [[ ${SKIPPED_TESTS} -gt 0 ]]; then
        echo -e "${YELLOW}Skipped:      ${SKIPPED_TESTS}${NC}"
    fi
    
    local pass_rate=$(echo "scale=1; ${PASSED_TESTS} * 100 / ${TOTAL_TESTS}" | bc)
    echo "Pass Rate:    ${pass_rate}%"
    echo ""
    
    # 保存报告到文件
    local report_file="${RESULTS_DIR}/report_$(date +%Y%m%d_%H%M%S).txt"
    cat > "${report_file}" << EOF
Qdrant Tool Test Report
=======================
Date: $(date)
Tool: ${QDRANT_TOOL}

Summary:
  Total Tests:  ${TOTAL_TESTS}
  Passed:       ${PASSED_TESTS}
  Failed:       ${FAILED_TESTS}
  Skipped:      ${SKIPPED_TESTS}
  Pass Rate:    ${pass_rate}%

Test Collection: ${TEST_COLLECTION}
EOF
    
    echo "Report saved to: ${report_file}"
    echo ""
    
    # 最终判断
    if [[ ${FAILED_TESTS} -eq 0 ]]; then
        echo -e "${GREEN}All tests passed!${NC}"
        return 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        return 1
    fi
}

# 主函数
main() {
    init
    
    # 运行所有测试套件
    test_version_and_help
    test_collection_management
    test_memory_add
    test_memory_search
    test_memory_update
    test_memory_delete
    test_file_import
    test_error_handling
    test_performance
    test_special_characters
    
    # 清理
    cleanup
    
    # 生成报告
    generate_report
}

# 设置 trap 以确保清理
trap cleanup EXIT

# 运行主函数
main "$@"
