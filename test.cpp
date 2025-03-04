#include <boost/context/execution_context.hpp>
#include <cstdio>
#include <iostream>
#include "coroutine.hpp"
// void func3(void *arg);
// void func2(ucontext_t& context);
// void func1(void *arg);


// void func3(void *arg)
// {
//     std::cout << "func1 调用" << std::endl;
//     std::cout << "func1 结束" << std::endl;
// }


// void func2(ucontext_t& context)
// {
//     ucontext_t now;
//     std::cout << "func2 调用" << std::endl;
//     getcontext(&now);
//     context.uc_link = &now;
//     setcontext(&context);
//     std::cout << "func2 结束" << std::endl;
// }


// void func1(void * arg)
// {
//     std::cout << "func1 调用" << std::endl;
//     std::cout << "func1 结束" << std::endl;
// }




// void context_test()
// {
//     char stack[1024*128];
//     ucontext_t now,next;

//     getcontext(&now); //获取当前上下文
//     int count = 1;
//     now.uc_stack.ss_sp = stack;//指定栈空间
//     now.uc_stack.ss_size = sizeof(stack);//指定栈空间大小
//     now.uc_stack.ss_flags = 0;
//     now.uc_link = &next;//设置后继上下文
//     count++;
//     std::cout << "count: " << count << "  " << "makecontext fix context_test() 中 now ,后续指向本身 now，激活时应该调用 func1" << std::endl;
//     makecontext(&now,(void (*)(void))func1,0);//修改上下文指向func1函数
//     // getcontext(&now); //获取当前上下文
//     //func2(now);
//     swapcontext(&next,&now);
//     std::cout << "now 重新加载 ，进行输出" << std::endl;
// }

// int main()
// {
//     context_test();
//     puts("main end");
//     return 0;
// }


// int main(){
//     int n = 9;
//     //创建一个协程,接收一个excution_context对象和一个整数参数
//     ctx::execution_context<int> source(
//         [n](ctx::execution_context<int> sink,int arg) mutable{
//             int a = 0,b = 1;
//             std::cout << "the first time exec coroutine!" << " n:" << n << ",arg:" << arg << std::endl;
//             while(n-->0){
//                 //执行sink(a)时暂停执行，返回当前的上下文和计算的值
//                 //result pair<ctx,int> 元组，第一个值为协程上下文，第二个值为返回值
//                 std::cout << "continue coroutine!" << " n:" << n << " a:" << a << std::endl;
//                 auto result = sink(a);
//                 sink = std::move(std::get<0>(result));
//                 auto next = a+b;
//                 a =b;
//                 b =next;
//             }
//             return sink;
//         }
//     );
//     for(int i = 0;i<11;i++){
//         if(source){
//             //协程恢复执行
//             std::cout << "jump out coroutine!" << std::endl;
//             // source 返回值 result 第一个为 source的上下文，第二个参数为source返回值
//             // 这里接收1 也是最后source执行完毕返回sink的返回值
//             auto result = source(1);
//             source = std::move(std::get<0>(result));
//             std::cout << "source return " << std::get<1>(result) << " " << std::endl<< std::endl;
//         }
//         else
//         {
//             std::cout << source << " end! " <<std::endl;
//         }
//     }
//     return 0;
// }
// namespace ctx2 = boost::context;

// // void (*pf)(int);


// // void add(int data){
// //     std::cout << "add function " << data << std::endl;
// // }

// // void sub(int data){
// //     std::cout << "sub function " << data << std::endl;
// // }


// // //第一个参数ctx是固定的，表明是会在当前context被suspend的时候自动切换resume至的context，通常来说是当前context的创建和调用者
// ctx2::execution_context<int> func1(ctx2::execution_context<int> ctx, int data) {
// 	std::cout << "func1: entered first time: " << data << std::endl;
// 	std::tie(ctx, data) = ctx(data+1);
// 	std::cout << "func1: entered second time: " << data << std::endl;
// 	std::tie(ctx, data) = ctx(data+1);
// 	std::cout << "func1: entered third time(atten): " << data << std::endl;
// 	return ctx;
// }

// std::tuple<boost::context::execution_context<int>,int> func2(boost::context::execution_context<int> ctx, int data) 
// {
//     std::cout << "func2: entered: " << data << std::endl;
//     return std::make_tuple(std::move(ctx), -3);
// }

// int main(){
// 	int data = 0;
// 	ctx2::execution_context< int > ctx(func1);
// 	//第一个值是suspend的context对象，其余部分是打包好的返回值
// 	std::tie(ctx, data) = ctx(data+1);
// 	std::cout << "func1: returned first time: " << data << std::endl;
// 	std::tie(ctx, data) = ctx(data+1);
// 	std::cout << "func1: returned second time: " << data << std::endl;
// 	// std::tie(ctx, data) = ctx(ctx2::exec_ontop_arg, func2, data+1);

// 	return 0;
// }


int main()
{
    CoroutinePool::getInstance()->start(5);
    return 0;
}


