#pragma once

namespace asio
{

template<typename Handler>
class binder0_t
{
public:
	binder0_t(Handler h): h_(h)
	{}
	void operator()()
	{
		h_();
	}

private:
	Handler h_;
};

template<typename Handler, typename A1>
class binder1_t
{
public:
	binder1_t(Handler h, A1 a1): h_(h), a1_(a1)
	{}
	void operator()()
	{
		h_(a1_);
	}

private:
	Handler h_;
	A1 a1_;
};

template<typename Handler, typename A1, typename A2>
class binder2_t
{
public:
	binder2_t(Handler h, A1 a1, A2 a2): h_(h), a1_(a1), a2_(a2)
	{}
	void operator()()
	{
		h_(a1_, a2_);
	}

private:
	Handler h_;
	A1 a1_;
	A2 a2_;
};

}
