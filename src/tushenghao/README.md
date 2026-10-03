## 构建与验证

构建：

```bash
cmake -S src/tushenghao -B build/tushenghao
cmake --build build/tushenghao
'''

测试：

'''bash
ctest --test-dir build/tushenghao --output-on-failure
'''

检查配置：

'''bash
./build/tushenghao/marker_app --check-config
'''

预期：100% tests passed + Config check passed