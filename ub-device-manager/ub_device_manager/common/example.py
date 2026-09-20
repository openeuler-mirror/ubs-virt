# Business Pydantic models
from pydantic import BaseModel

from ub_device_manager.common.async_task_framework import BaseExceptionHandler, AsyncTask, AsyncTaskChain, Context


class UserReq(BaseModel):
    user_id: int
    name: str

class UserMid(BaseModel):
    user_id: int
    name: str
    age: int

class UserResp(BaseModel):
    success: bool
    user_info: UserMid

# Custom task 1
class FillAgeTask(AsyncTask[UserReq, UserMid]):
    async def execute(self, input_data: UserReq) -> UserMid:
        # Simulate a database query.
        self.context.set("query_user_id", input_data.user_id)
        return UserMid(user_id=input_data.user_id, name=input_data.name, age=20)

# Custom task 2
class BuildRespTask(AsyncTask[UserMid, UserResp]):
    async def execute(self, input_data: UserMid) -> UserResp:
        return UserResp(success=True, user_info=input_data)

    async def when_raise_exception(self, exception: Exception):
        print(f"Task 2 caught the exception itself: {exception}")
        raise exception

# Global exception handler
class GlobalTaskExceptionHandler(BaseExceptionHandler):
    async def handle_exception(self, exception: Exception, context: Context) -> None:
        print(f"Global exception caught | context={context} | err={str(exception)}")



import asyncio

async def main():
    # Build initial input.
    init_req = UserReq(user_id=1001, name="ZhangSan")
    # Build the task chain.
    chain = AsyncTaskChain[UserReq, UserResp](initial_input=init_req) \
        .with_context({"trace_id": "trace-xxxx-1234"}) \
        .set_global_exception_handler(GlobalTaskExceptionHandler) \
        .apply_async_task(FillAgeTask) \
        .apply_async_task(BuildRespTask)
    # Run the chain.
    resp = await chain.run_chain()
    print("Chain result: ", resp)
    print("Chain context: ", chain._context)

if __name__ == "__main__":
    asyncio.run(main())
