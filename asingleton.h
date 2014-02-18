#ifndef ASINGLETON_H
#define ASINGLETON_H

template<class T>
class ASingleton {
    public:
        //! Функция возврата объекта синглтона.
        static T &instance() {static T object; return object;}

    protected:
        //! Конструктор.
        ASingleton() {}

        //! Деструктор.
        virtual ~ASingleton() {}

    private:
        //! Конструктор копирования.
        ASingleton(const ASingleton &other);

        //! Оператор присваивания.
        ASingleton &operator=(const ASingleton &other);

};

#endif
