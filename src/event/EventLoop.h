#ifndef __EVENT_LOOP_H
#define __EVENT_LOOP_H

#include <mutex>
#include <thread>
#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include "PipeWrap.h"
#include "Logger.h"
#include "Semaphore.h"
//#include "Util/List.h"
//#include "Thread/TaskExecutor.h"
//#include "Thread/ThreadPool.h"
//#include "Network/Buffer.h"
//#include "Network/BufferSock.h"

#if defined(__linux__) || defined(__linux)
#define HAS_EPOLL
#endif //__linux__

#if defined(__APPLE__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define HAS_KQUEUE
#endif // __APPLE__

namespace chw {

class EventLoop : public std::enable_shared_from_this<EventLoop> {
public:
    friend class TaskExecutorGetterImp;

    using Ptr = std::shared_ptr<EventLoop>;
    using PollEventCB = std::function<void(int event)>;
    using PollCompleteCB = std::function<void(bool success)>;
    using DelayTask = TaskCancelableImp<uint64_t(void)>;

    typedef enum {
        Event_Read = 1 << 0, //读事件
        Event_Write = 1 << 1, //写事件
        Event_Error = 1 << 2, //错误事件
        Event_LT = 1 << 3,//水平触发
    } Poll_Event;

    
    ~EventLoop();

    /**
     * 添加事件监听
     * @param fd 监听的文件描述符
     * @param event 事件类型，例如 Event_Read | Event_Write
     * @param cb 事件回调functional
     * @return -1:失败，0:成功
     */
    int addEvent(int fd, int event, PollEventCB cb);

    /**
     * 删除事件监听
     * @param fd 监听的文件描述符
     * @param cb 删除成功回调functional
     * @return -1:失败，0:成功
     */
    int delEvent(int fd, PollCompleteCB cb = nullptr);

    /**
     * 修改监听事件类型
     * @param fd 监听的文件描述符
     * @param event 事件类型，例如 Event_Read | Event_Write
     * @return -1:失败，0:成功
     */
    int modifyEvent(int fd, int event, PollCompleteCB cb = nullptr);

    /**
     * 异步执行任务
     * @param task 任务
     * @param may_sync 如果调用该函数的线程就是本对象的轮询线程，那么may_sync为true时就是同步执行任务
     * @return 是否成功，一定会返回true

     */
    Task::Ptr async(TaskIn task, bool may_sync = true);

    /**
     * 同async方法，不过是把任务打入任务列队头，这样任务优先级最高
     * @param task 任务
     * @param may_sync 如果调用该函数的线程就是本对象的轮询线程，那么may_sync为true时就是同步执行任务
     * @return 是否成功，一定会返回true
     */
    Task::Ptr async_first(TaskIn task, bool may_sync = true);

    /**
     * 判断执行该接口的线程是否为本对象的轮询线程
     * @return 是否为本对象的轮询线程
     */
    bool isCurrentThread();

    /**
     * 延时执行某个任务
     * @param delay_ms 延时毫秒数
     * @param task 任务，返回值为0时代表不再重复任务，否则为下次执行延时，如果任务中抛异常，那么默认不重复任务
     * @return 可取消的任务标签
     */
    DelayTask::Ptr doDelayTask(uint64_t delay_ms, std::function<uint64_t()> task);

    /**
     * 获取当前线程关联的Poller实例
     */
    static EventLoop::Ptr getCurrentPoller();

    // /**
    //  * 获取当前线程下所有socket共享的读缓存
    //  * Gets the shared read buffer for all sockets in the current thread
     
    //  * [AUTO-TRANSLATED:2796f458]
    //  */
    // SocketRecvBuffer::Ptr getSharedBuffer(bool is_udp);

    /**
     * 获取poller线程id
     */
    std::thread::id getThreadId() const;

    /**
     * 获取线程名
     */
    const std::string& getThreadName() const;

    /**
     * @brief 创建一个EventLoop
     * 
     * @param full_name     线程名
     * @param priority      线程优先级
     * @param enable_cpu_affinity   是否绑定cpu
     * @param cpu_index     绑定的cpu号
     * @return EventLoop::Ptr 
     */
    static EventLoop::Ptr addPoller(const std::string& full_name, int priority = PRIORITY_HIGHEST, bool enable_cpu_affinity = false, int32_t cpu_index = 0)
    {
        chw::EventLoop::Ptr poller = std::make_shared<chw::EventLoop>(full_name);
        poller->runLoop(false, true);
        poller->async([cpu_index, full_name, priority, enable_cpu_affinity]() {
            // 设置线程优先级
            setPriority((Priority)priority);
            // 设置线程名
            setThreadName(full_name.data());
            // 设置cpu亲和性
            if (enable_cpu_affinity) {
                setThreadAffinity(cpu_index);
            }
        });

        return poller;
    }

public:
    EventLoop(std::string name);

    /**
     * 执行事件轮询
     * @param blocked 是否用执行该接口的线程执行轮询
     * @param ref_self 是记录本对象到thread local变量
     */
    void runLoop(bool blocked, bool ref_self);

    /**
     * 内部管道事件，用于唤醒轮询线程用
     */
    void onPipeEvent(bool flush = false);

    /**
     * 切换线程并执行任务
     * @param task
     * @param may_sync
     * @param first
     * @return 可取消的任务本体，如果已经同步执行，则返回nullptr
     */
    Task::Ptr async_l(TaskIn task, bool may_sync = true, bool first = false);

    /**
     * 结束事件轮询
     * 需要指出的是，一旦结束就不能再次恢复轮询线程
     */
    void shutdown();

    /**
     * 刷新延时任务
     */
    uint64_t flushDelayTask(uint64_t now);

    /**
     * 获取select或epoll休眠时间
     */
    uint64_t getMinDelay();

    /**
     * 添加管道监听事件
     */
    void addEventPipe();

private:
    class ExitException : public std::exception {};

private:
    //标记loop线程是否退出
    bool _exit_flag;
    //线程名
    std::string _name;
    //当前线程下，所有socket共享的读缓存  [AUTO-TRANSLATED:6ce70017]
    //当前线程下，所有socket共享的读缓存
// Shared read buffer for all sockets under the current thread
    // std::weak_ptr<SocketRecvBuffer> _shared_buffer[2];

    //执行事件循环的线程
    std::thread *_loop_thread = nullptr;
    //通知事件循环的线程已启动
    Semaphore _sem_run_started;

    //内部事件管道
    PipeWrap _pipe;
    //从其他线程切换过来的任务
    std::mutex _mtx_task;
    List<Task::Ptr> _list_task;

    //保持日志可用
    Logger::Ptr _logger;

#if defined(HAS_EPOLL) || defined(HAS_KQUEUE)
    // epoll和kqueue相关  [AUTO-TRANSLATED:84d2785e]
    //epoll和kqueue相关
// epoll and kqueue related
    int _event_fd = -1;
    std::unordered_map<int, std::shared_ptr<PollEventCB> > _event_map;
#else
    //select相关
    struct Poll_Record {
        using Ptr = std::shared_ptr<Poll_Record>;
        int fd;
        int event;
        int attach;
        PollEventCB call_back;
    };
    std::unordered_map<int, Poll_Record::Ptr> _event_map;
#endif //HAS_EPOLL
    std::unordered_set<int> _event_cache_expired;

    //定时器
    std::multimap<uint64_t, DelayTask::Ptr> _delay_task_map;
};

}  // namespace chw
#endif //__EVENT_LOOP_H
