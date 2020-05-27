/* Interface that should be implemented by IO classes */

class IoInterface
{
  public:
    virtual void cmd(const uint8_t cmd) = 0;
    virtual void data(uint8_t data);
    virtual void data(const uint8_t *data, int len);
    virtual void reset();
    virtual void init(uint8_t frequency);
};