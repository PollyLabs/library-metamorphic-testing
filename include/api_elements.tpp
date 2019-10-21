template<typename T>
std::string
PrimitiveObject<T>::toStr() const
{
    const PrimitiveType* prim_type =
        dynamic_cast<const PrimitiveType*>(this->getType());
    if (prim_type->getTypeEnum() == STRING)
    {
        return fmt::format("\"{}\"", this->data);
    }
    else if (prim_type->getTypeEnum() == CHAR)
    {
        return fmt::format("'{}'", this->data);
    }
    return fmt::format("{}", this->data);
}

