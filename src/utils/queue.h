#pragma once

namespace util
{
	template <typename _Ty, int N>
	struct Queue
	{
		Queue() = default;
		~Queue() = default;

		template <typename T>
		inline bool push(const T& value)
		{
			static_assert(sizeof(T) >= sizeof(_Ty));
			constexpr int TRATIO = sizeof(T) / sizeof(_Ty);

			if (count + TRATIO <= N)
			{
				_Ty* dbuf = (_Ty*)&value;
				for (int i = 0; i < TRATIO; i++)
				{
					buffer[rear] = dbuf[i];
					rear = (rear + 1) % N;
				}
				
				count += TRATIO;
				return true;
			}

			return false;
		}

		template <typename T = _Ty>
		inline bool pop()
		{
			static_assert(sizeof(T) >= sizeof(_Ty));
			constexpr int TRATIO = sizeof(T) / sizeof(_Ty);

			if (count >= TRATIO)
			{
				front = (front + TRATIO) % N;
				count -= TRATIO;
				return true;
			}

			return false;
		}

		template <typename T>
		inline bool read(T* out)
		{
			static_assert(sizeof(T) >= sizeof(_Ty));
			constexpr int TRATIO = sizeof(T) / sizeof(_Ty);

			if (count >= TRATIO)
			{
				_Ty* dbuf = (_Ty*)out;
				auto peek = front;
				for (int i = 0; i < TRATIO; i++)
				{
					dbuf[i] = buffer[peek];
					peek = (peek + 1) % N;
				}
				return true;
			}

			return false;
		}

		inline bool empty() const
		{
			return !count;
		}

		template <typename T = _Ty>
		inline int size() const
		{
			return count * sizeof(_Ty) / sizeof(T);
		}

		_Ty buffer[N] = {};
		int front = 0, rear = 0, count = 0;
	};
}