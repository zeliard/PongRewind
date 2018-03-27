#pragma once

template <typename T>
struct is_shared_ptr
{
	const static bool value = false;
};

template <typename T>
struct is_shared_ptr<std::shared_ptr<T> >
{
	const static bool value = true;
};

template <typename T>
struct is_shared_ptr<std::shared_ptr<const T> >
{
	const static bool value = true;
};

template <typename T>
struct is_shared_ptr<const std::shared_ptr<T> >
{
	const static bool value = true;
};

template <typename T>
struct is_shared_ptr<const std::shared_ptr<const T> >
{
	const static bool value = true;
};

