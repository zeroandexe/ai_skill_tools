#!/bin/bash
#
# 简单的 Bash 测试框架
#

# 计数器
TESTS_RUN=0
TESTS_PASSED=0
TESTS_FAILED=0

# 颜色
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

# 断言函数
assert_equals() {
    local expected="$1"
    local actual="$2"
    local message="$3"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if [[ "${expected}" == "${actual}" ]]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "  ${GREEN}✓${NC} ${message}"
        return 0
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "  ${RED}✗${NC} ${message}"
        echo "    Expected: ${expected}"
        echo "    Actual: ${actual}"
        return 1
    fi
}

assert_not_equals() {
    local unexpected="$1"
    local actual="$2"
    local message="$3"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if [[ "${unexpected}" != "${actual}" ]]; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "  ${GREEN}✓${NC} ${message}"
        return 0
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "  ${RED}✗${NC} ${message}"
        echo "    Unexpected: ${unexpected}"
        return 1
    fi
}

assert_true() {
    local condition="$1"
    local message="$2"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if eval "${condition}"; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "  ${GREEN}✓${NC} ${message}"
        return 0
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "  ${RED}✗${NC} ${message}"
        return 1
    fi
}

assert_false() {
    local condition="$1"
    local message="$2"
    
    TESTS_RUN=$((TESTS_RUN + 1))
    
    if ! eval "${condition}"; then
        TESTS_PASSED=$((TESTS_PASSED + 1))
        echo -e "  ${GREEN}✓${NC} ${message}"
        return 0
    else
        TESTS_FAILED=$((TESTS_FAILED + 1))
        echo -e "  ${RED}✗${NC} ${message}"
        return 1
    fi
}

# 打印测试报告
print_test_report() {
    echo ""
    echo "Test Report:"
    echo "  Total:  ${TESTS_RUN}"
    echo -e "  Passed: ${GREEN}${TESTS_PASSED}${NC}"
    echo -e "  Failed: ${RED}${TESTS_FAILED}${NC}"
    
    if [[ ${TESTS_FAILED} -eq 0 ]]; then
        return 0
    else
        return 1
    fi
}

# 如果直接运行此脚本，显示帮助
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    echo "This is a test framework. Source it in your test scripts:"
    echo "  source test_framework.sh"
    echo ""
    echo "Available assertions:"
    echo "  assert_equals expected actual message"
    echo "  assert_not_equals unexpected actual message"
    echo "  assert_true condition message"
    echo "  assert_false condition message"
    echo "  print_test_report"
fi
