#include <iostream>
#include <string> // 为 std::string 添加
#include <entt/entt.hpp>

struct TestEvent {
    std::string message;
};

// 为事件处理器定义一个自由函数
void eventHandler(TestEvent &event) {
    std::cout << "Received event: " << event.message << std::endl;
}

int main() {
    entt::registry registry;
    // 从注册表的上下文中获取或创建 dispatcher 实例
    // auto& dispatcher 会正确地成为该实例的引用
    auto &dispatcher = registry.ctx().emplace<entt::dispatcher>();

    // 注册事件监听器 (sink)
    // 使用自由函数 eventHandler 的地址作为模板非类型参数进行连接。
    // 这是连接自由函数到 EnTT dispatcher 的一种可靠方式，尤其是在某些编译器或库版本下。
    dispatcher.sink<TestEvent>().connect<&eventHandler>();

    // 将事件入队
    // TestEvent 会使用 "Hello from EnTT observer!" 作为 message 构造。
    dispatcher.enqueue<TestEvent>({"Hello from EnTT observer!"});

    // 更新 dispatcher，处理队列中的所有事件
    // 这时，已连接的监听器会被调用。
    dispatcher.update();

    return 0;
}
